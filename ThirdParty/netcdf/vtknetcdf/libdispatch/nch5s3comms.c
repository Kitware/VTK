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
 * Source for S3 Communications module
 * ***NOT A FILE DRIVER***
 * Provide functions and structures required for interfacing with Amazon
 * Simple Storage Service (S3).
 * Provide S3 object access as if it were a local file.
 * Connect to remote host, send and receive HTTP requests and responses
 * as part of the AWS REST API, authenticating requests as appropriate.
 * Programmer: Jacob Smith
 *             2017-11-30
 *****************************************************************************/

/**
 * Unidata Changes:
 * Derived from HDF5-1.14.0 H5FDs3comms.[ch]
 * Modified to support Write operations and support NCZarr.
 * Primary Changes:
 * - rename H5FD_s3comms to NCH5_s3comms to avoid name conflicts
 * - Remove HDF5 dependencies
 * - Support zmap API
 *
 * Note that this code is very ugly because it is the bastard
 * child of the HDF5 coding style and the NetCDF-C coding style
 * and some libcurl as well.
 * 
 * A note about the nccurl_hmac.c and nccurl_sha256.c files.
 * The code in this file depends on having access to two
 * cryptographic functions:
 * 1. HMAC signing function 
 * 2. SHA256 digest function
 * 
 * There are a number of libraries providing these functions.
 * For example, OPENSSL, WOLFSSL, GNUTLS, Windows crypto package
 * etc.  It turns out that libcurl has identified all of these
 * possible sources and set up a wrapper to handle the
 * possibilities.  So, this code copies the libcurl wrapper to
 * inherit its multi-source capabilities.
 *
 * Author: Dennis Heimbigner
 * Creation Date: 2/12/2023
 * Last Modified: 5/1/2023
 */

/****************/
/* Module Setup */
/****************/

/***********/
/* Headers */
/***********/

/*****************/
#include "config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#include <assert.h>

#include "nccurl_sha256.h"
#include "nccurl_hmac.h"

/* Necessary S3 headers */
#include <curl/curl.h>
//#include <openssl/evp.h>
//#include <openssl/hmac.h>
//#include <openssl/sha.h>

#include "netcdf.h"
#include "ncuri.h"
#include "ncutil.h"

/*****************/

#include "ncs3sdk.h"
#include "nch5s3comms.h" /* S3 Communications */

/****************/
/* Local Macros */
/****************/

#undef TRACING
#undef DEBUG

#define SUCCEED NC_NOERR
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* enable debugging */
#define S3COMMS_DEBUG 0
#define S3COMMS_DEBUG_TRACE 0

/* manipulate verbosity of CURL output
 * operates separately from S3COMMS_DEBUG
 * 0 -> no explicit curl output
 * 1 -> on error, print failure info to stderr
 * 2 -> in addition to above, print information for all performs; sets all
 *      curl handles with CURLOPT_VERBOSE
 */
#define S3COMMS_CURL_VERBOSITY 0

/* Apparently Apple/OSX C Compiler does not (yet) accept __VA_OPT__(,),
   so we have to case it out (ugh!)
*/
#if S3COMMS_CURL_VERBOSITY > 1
#define HDONE_ERROR(ignore1,ncerr,ignore2,msg) do {ret_value=report(ncerr,__func__,__LINE__,msg);} while(0)
#define HDONE_ERRORVA(ignore1,ncerr,ignore2,msg,...) do {ret_value=report(ncerr,__func__,__LINE__,msg, __VA_ARGS__);} while(0)
#define HGOTO_ERROR(ignore1,ncerr,ignore2,msg,...) do {ret_value=report(ncerr,__func__,__LINE__,msg); goto done;} while(0)
#define HGOTO_ERRORVA(ignore1,ncerr,ignore2,msg,...) do {ret_value=report(ncerr,__func__,__LINE__,msg, __VA_ARGS__); goto done;} while(0)
#else /*S3COMMS_CURL_VERBOSITY*/
#define HDONE_ERROR(ignore1,ncerr,ignore2,msg,...) do {ret_value=(ncerr);} while(0)
#define HDONE_ERRORVA(ignore1,ncerr,ignore2,msg,...) HDONE_ERROR(ignore1,ncerr,ignore2,msg)
#define HGOTO_ERROR(ignore1,ncerr,ignore2,msg,...) do {ret_value=(ncerr);; goto done;} while(0)
#define HGOTO_ERRORVA(ignore1,ncerr,ignore2,msg,...) HGOTO_ERROR(ignore1,ncerr,ignore2,msg)
#endif /*S3COMMS_CURL_VERBOSITY*/

/* size to allocate for "bytes=<first_byte>[-<last_byte>]" HTTP Range value
 */
#define S3COMMS_MAX_RANGE_STRING_SIZE 128

#define SNULL(x) ((x)==NULL?"NULL":(x))
#define INULL(x) ((x)==NULL?-1:(int)(*x))

#ifdef TRACING
#define TRACE(level,fmt,...) s3trace((level),__func__,fmt,##__VA_ARGS__)
#define TRACEMORE(level,fmt,...) s3tracemore((level),fmt,##__VA_ARGS__)
#define UNTRACE(e) s3untrace(__func__,NCTHROW(e),NULL)
#define UNTRACEX(e,fmt,...) s3untrace(__func__,NCTHROW(e),fmt,##__VA_ARGS__)
#else
#define TRACE(level,fmt,...)
#define TRACEMORE(level,fmt,...)
#define UNTRACE(e) (e)
#define UNTRACEX(e,fmt,...) (e)
#endif

#ifdef TRACING
static struct S3LOGGLOBAL {
    FILE* stream;
    int depth;
    struct Frame {
	const char* fcn;
	int level;
	int depth;
    } frames[1024];
} s3log_global = {NULL,0};

static int
s3breakpoint(int err)
{
    return err;
}

static void
s3vtrace(int level, const char* fcn, const char* fmt, va_list ap)
{
    struct Frame* frame;
    if(s3log_global.stream == NULL) s3log_global.stream = stderr;
    if(fcn != NULL) {
        frame = &s3log_global.frames[s3log_global.depth];
        frame->fcn = fcn;
        frame->level = level;
        frame->depth = s3log_global.depth;
    }
    {
	if(fcn != NULL)
            fprintf(s3log_global.stream,"%s: (%d): %s:","Enter",level,fcn);
        if(fmt != NULL)
            vfprintf(s3log_global.stream, fmt, ap);
        fprintf(s3log_global.stream, "\n" );
        fflush(s3log_global.stream);
    }
    if(fcn != NULL) s3log_global.depth++;
}

static void
s3trace(int level, const char* fcn, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    s3vtrace(level,fcn,fmt,args);
    va_end(args);
}

static void
s3tracemore(int level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    s3vtrace(level,NULL,fmt,args);
    va_end(args);
}

static int
s3untrace(const char* fcn, int err, const char* fmt, ...)
{
    va_list args;
    struct Frame* frame;
    va_start(args, fmt);
    if(s3log_global.depth == 0) {
	fprintf(s3log_global.stream,"*** Unmatched untrace: %s: depth==0\n",fcn);
	goto done;
    }
    s3log_global.depth--;
    frame = &s3log_global.frames[s3log_global.depth];
    if(frame->depth != s3log_global.depth || strcmp(frame->fcn,fcn) != 0) {
	fprintf(s3log_global.stream,"*** Unmatched untrace: fcn=%s expected=%s\n",frame->fcn,fcn);
	goto done;
    }
    {
        fprintf(s3log_global.stream,"%s: (%d): %s: ","Exit",frame->level,frame->fcn);
	if(err)
	    fprintf(s3log_global.stream,"err=(%d) '%s':",err,nc_strerror(err));
        if(fmt != NULL)
            vfprintf(s3log_global.stream, fmt, args);
        fprintf(s3log_global.stream, "\n" );
        fflush(s3log_global.stream);
    }
done:
    va_end(args);
    if(err != 0)
        return s3breakpoint(err);
    else
	return err;
}

#endif


/******************/
/* Local Decls*/
/******************/

#define S3COMMS_VERB_MAX 16

/********************/
/* Local Structures */
/********************/

/* Provide a single, unified argument for curl callbacks */
/* struct s3r_cbstruct
 * Structure passed to curl callback
 */
struct s3r_cbstruct {
    unsigned long magic;
    VString*    data;
    const char* key; /* headcallback: header search key */
    size_t      pos; /* readcallback: write from this point in data */
};
#define S3COMMS_CALLBACK_STRUCT_MAGIC 0x28c2b2ul

/********************/
/* Local Prototypes */
/********************/

/* Forward */
static int NCH5_s3comms_s3r_execute(s3r_t *handle, const char* url, HTTPVerb verb, const char* byterange, const char* header, const char** otherheaders, long* httpcodep, VString* data);
static size_t curlwritecallback(char *ptr, size_t size, size_t nmemb, void *userdata);
static size_t curlheadercallback(char *ptr, size_t size, size_t nmemb, void *userdata);
static int curl_reset(s3r_t* handle);
static int perform_request(s3r_t* handle, long* httpcode);
static int build_request(s3r_t* handle, NCURI* purl, const char* byterange, const char** otherheaders, VString* payload, HTTPVerb verb);
static int request_setup(s3r_t* handle, const char* url, HTTPVerb verb, struct s3r_cbstruct*);
static int validate_handle(s3r_t* handle, const char* url);
static int validate_url(NCURI* purl);
static int build_range(size_t offset, size_t len, char** rangep);
static const char* verbtext(HTTPVerb verb);
static int trace(CURL* curl, int onoff);
static int sortheaders(VList* headers);
static int httptonc(long httpcode);
static void hrb_node_free(hrb_node_t *node);

#if S3COMMS_DEBUG_HRB
static void dumphrbnodes(VList* nodes);
static void dumphrb(hrb_t* hrb);
#endif

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*************/
/* Functions */
/*************/

#if S3COMMS_CURL_VERBOSITY > 0
static void
nch5breakpoint(int stat)
{
    if(stat == -78) abort();
    ncbreakpoint(stat);
}

static int
report(int stat, const char* fcn, int lineno, const char* fmt, ...)
{
    va_list args;
    char bigfmt[1024];

    if(stat == NC_NOERR) goto done;
    snprintf(bigfmt,sizeof(bigfmt),"(%d)%s ; fcn=%s line=%d ; %s",stat,nc_strerror(stat),fcn,lineno,fmt);
    va_start(args,fmt);
    ncvlog(NCLOGERR,bigfmt,args);
    nch5breakpoint(stat);

done:
    va_end(args);
    return stat;
}
#endif

/*----------------------------------------------------------------------------
 * Function: curlwritecallback()
 * Purpose:
 *     Function called by CURL to write received data.
 *     Writes bytes to `userdata`.
 *     Internally manages number of bytes processed.
 * Return:
 *     - Number of bytes processed.
 *         - Should equal number of bytes passed to callback.
 *         - Failure will result in curl error: CURLE_WRITE_ERROR.
 * Programmer: Jacob Smith
 *             2017-08-17
 *----------------------------------------------------------------------------
 */
static size_t
curlwritecallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct s3r_cbstruct *sds     = (struct s3r_cbstruct *)userdata;
    size_t                 product = (size * nmemb);
    size_t                 written = 0;

    if (sds->magic != S3COMMS_CALLBACK_STRUCT_MAGIC)
        return written;

    if (product > 0) { 
        vsappendn(sds->data,ptr,product);
        written = product;
    }

    return written;
} /* end curlwritecallback() */

/*----------------------------------------------------------------------------
 * Function: curlreadcallback()
 * Purpose:
 *     Function called by CURL to write PUT data.
 *     Reads bytes from `userdata`.
 *     Internally manages number of bytes processed.
 * Return:
 *     - Number of bytes processed.
 *         - Should equal number of bytes passed to callback.
 *         - Failure will result in curl error: CURLE_WRITE_ERROR.
 * Programmer: Dennis Heimbigner
 *----------------------------------------------------------------------------
 */
static size_t
curlreadcallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct s3r_cbstruct *sds     = (struct s3r_cbstruct *)userdata;
    size_t                 product = (size * nmemb);
    size_t                 written = 0;
    size_t                 avail = 0;
    size_t                 towrite = 0;

    if (sds->magic != S3COMMS_CALLBACK_STRUCT_MAGIC)
        return CURL_READFUNC_ABORT;

    avail = (vslength(sds->data) - sds->pos);
    towrite = (product > avail ? avail : product);
    if (towrite > 0) {
	const char* data = vscontents(sds->data);
	memcpy(ptr,&data[sds->pos],towrite);
    }
    sds->pos += towrite;
    written = towrite;

    return written;
} /* end curlreadcallback() */

/*----------------------------------------------------------------------------
 * Function: curlheadercallback()
 * Purpose:
 *     Function called by CURL to write headers.
 *     Writes target header line to value field;
 *     Internally manages number of bytes processed.
 * Return:
 *     - Number of bytes processed.
 *         - Should equal number of bytes passed to callback.
 *         - Failure will result in curl error: CURLE_WRITE_ERROR.
 * Programmer: Dennis Heimbigner
 *             2017-08-17
 *----------------------------------------------------------------------------
 */
static size_t
curlheadercallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct s3r_cbstruct *sds = (struct s3r_cbstruct *)userdata;
    size_t len = (size * nmemb);
    char* line = ptr;
    size_t i,j;

    if (sds->magic != S3COMMS_CALLBACK_STRUCT_MAGIC)
        return 0;
    if(vslength(sds->data) > 0)
        goto done; /* already found */

    /* skip leading white space */
    for(j=0,i=0;i<len;i++) {if(!isspace(line[i])) {j = i; break;}}
    line = line + j;
    len -= j;

    if(sds->key && strncasecmp(line,sds->key,strlen(sds->key)) == 0) {
        vsappendn(sds->data,line,len);
    }

done:
    return size * nmemb;

} /* end curlwritecallback() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_hrb_node_insert()
 * Purpose:
 *     Insert elements in a field node list.
 *     `name` cannot be null; will return FAIL and list will be unaltered.
 *     Entries are accessed via the lowercase representation of their name:
 *     "Host", "host", and "hOSt" would all access the same node,
 *     but name's case is relevant in HTTP request output.
 *----------------------------------------------------------------------------
 */

int
NCH5_s3comms_hrb_node_insert(VList* list, const char *name, const char *value)
{
    size_t      i          = 0;
    int         ret,ret_value  = SUCCEED;
    size_t catlen, namelen;
    size_t catwrite;
    char* lowername = NULL;
    char* nvcat = NULL;
    hrb_node_t* new_node = NULL;

#if S3COMMS_DEBUG_HRB
    fprintf(stdout, "called NCH5_s3comms_hrb_node_insert.");
    printf("NAME: %s\n", name);
    printf("VALUE: %s\n", value);
    printf("LIST:\n->");
    dumphrbnodes(list);
    fflush(stdout);
#endif

    if (name == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to operate on null name");
    namelen = nulllen(name);

    /* get lowercase name */
    lowername = (char *)malloc(sizeof(char) * (namelen + 1));
    if (lowername == NULL)
        HGOTO_ERROR(H5E_RESOURCE, NC_ENOMEM, FAIL, "cannot make space for lowercase name copy.");
    for (i = 0; i < namelen; i++)
        lowername[i] = (char)tolower((int)name[i]);
    lowername[namelen] = 0;

    if(value == NULL) value = "";

    /* create new_node */
    new_node = (hrb_node_t *)calloc(1,sizeof(hrb_node_t));
    if (new_node == NULL)
        HGOTO_ERROR(H5E_RESOURCE, NC_ENOMEM, FAIL, "cannot make space for new set.");
    new_node->magic     = S3COMMS_HRB_NODE_MAGIC;
    new_node->name      = strdup(name);
    new_node->value     = strdup(value);

    catlen   = namelen + strlen(value) + 2; /* +2 from ": " */
    catwrite = catlen + 3;             /* 3 not 1 to quiet compiler warning */
    nvcat = (char *)malloc(catwrite);
    if (nvcat == NULL)
	HGOTO_ERROR(H5E_RESOURCE, NC_ENOMEM, FAIL, "cannot make space for concatenated string.");
    ret = snprintf(nvcat, catwrite, "%s: %s", lowername, value);
    if (ret < 0 || (size_t)ret > catlen)
        HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "cannot concatenate `%s: %s", name, value);
    assert(catlen == nulllen(nvcat));
    new_node->cat       = nvcat; nvcat = NULL;

    new_node->lowername = lowername; lowername = NULL;

    vlistpush(list,new_node); new_node = NULL;

done:
    /* clean up */
    if (nvcat != NULL) free(nvcat);
    if (lowername != NULL) free(lowername);
    hrb_node_free(new_node);
    return (ret_value);
}


/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_hrb_destroy()
 * Purpose:
 *    Destroy and free resources _directly_ associated with an HTTP Buffer.
 *    Takes a pointer to pointer to the buffer structure.
 *    This allows for the pointer itself to be NULLed from within the call.
 *    If buffer or buffer pointer is NULL, there is no effect.
 *    Headers list at `first_header` is not touched.
 *    - Programmer should re-use or destroy `first_header` pointer
 *      (hrb_node_t *) as suits their purposes.
 *    - Recommend fetching prior to destroy()
 *      e.g., `reuse_node = hrb_to_die->first_header; destroy(hrb_to_die);`
 *      or maintaining an external reference.
 *    - Destroy node/list separately as appropriate
 *    - Failure to account for this will result in a memory leak.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - successfully released buffer resources
 *         - if `buf` is NULL or `*buf` is NULL, no effect
 *     - FAILURE: `FAIL`
 *         - `buf->magic != S3COMMS_HRB_MAGIC`
 * Programmer: Jacob Smith
 *             2017-07-21
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_hrb_destroy(hrb_t *buf)
{
    int ret_value = SUCCEED;
    size_t i;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_hrb_destroy.\n");
#endif

    if(buf == NULL) return ret_value;

    if (buf->magic != S3COMMS_HRB_MAGIC)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "pointer's magic does not match.");
    free(buf->version);
    free(buf->resource);
    buf->magic += 1ul;
    vsfree(buf->body);
    for(i=0;i<vlistlength(buf->headers);i++) {
        hrb_node_t* node = (hrb_node_t*)vlistget(buf->headers,i);
	hrb_node_free(node);
    }
    vlistfree(buf->headers);
    free(buf);
done:
    return (ret_value);
} /* end NCH5_s3comms_hrb_destroy() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_hrb_init_request()
 * Purpose:
 *     Create a new HTTP Request Buffer
 *     All non-null arguments should be null-terminated strings.
 *     If `verb` is NULL, defaults to "GET".
 *     If `http_version` is NULL, defaults to "HTTP/1.1".
 *     `resource` cannot be NULL; should be string beginning with slash
 *     character ('/').
 *     All strings are copied into the structure, making them safe from
 *     modification in source strings.
 * Return:
 *     - SUCCESS: pointer to new `hrb_t`
 *     - FAILURE: `NULL`
 * Programmer: Jacob Smith
 *             2017-07-21
 *----------------------------------------------------------------------------
 */
hrb_t *
NCH5_s3comms_hrb_init_request(const char *_resource, const char *_http_version)
{
    hrb_t *request   = NULL;
    char  *res       = NULL;
    size_t reslen    = 0;
    int ret_value    = SUCCEED;
    char  *vrsn      = NULL;
    size_t vrsnlen   = 0;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_hrb_init_request.\n");
#endif

    if (_resource == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "resource string cannot be null.");

    /* populate valid NULLs with defaults */
    if (_http_version == NULL)
        _http_version = "HTTP/1.1";

    /* malloc space for and prepare structure */
    request = (hrb_t *)malloc(sizeof(hrb_t));
    if (request == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "no space for request structure");
    request->magic        = S3COMMS_HRB_MAGIC;
    request->body         = vsnew();
    request->headers	  = vlistnew();

    /* malloc and copy strings for the structure */
    reslen = nulllen(_resource);

    if (_resource[0] == '/') {
        res = (char *)malloc(sizeof(char) * (reslen + 1));
        if (res == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "no space for resource string");
        memcpy(res, _resource, (reslen + 1));
    }
    else {
        res = (char *)malloc(sizeof(char) * (reslen + 2));
        if (res == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "no space for resource string");
        *res = '/';
        memcpy((&res[1]), _resource, (reslen + 1));
        assert((reslen + 1) == nulllen(res));
    } /* end if (else resource string not starting with '/') */

    vrsnlen = nulllen(_http_version) + 1;
    vrsn    = (char *)malloc(sizeof(char) * vrsnlen);
    if (vrsn == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "no space for http-version string");
    strncpy(vrsn, _http_version, vrsnlen);

    /* place new copies into structure */
    request->resource = res;
    request->version  = vrsn;

done:
    /* if there is an error, clean up after ourselves */
    if (ret_value != SUCCEED) {
        if (request != NULL)
            free(request);
        if (vrsn != NULL)
            free(vrsn);
        if (res != NULL)
            free(res);
        request = NULL;
    }

    (void)(ret_value);
    return request;
} /* end NCH5_s3comms_hrb_init_request() */

#if S3COMMS_DEBUG_HRB
static void
dumphrbnodes(VList* nodes)
{
    int i;
    if(nodes != NULL) {
	fprintf(stderr,"\tnodes={\n");
	for(i=0;i<vlistlength(nodes);i++) {
	    hrb_node_t* node = (hrb_node_t*)vlistget(nodes,i);
	    fprintf(stderr,"\t\t[%2d] %s=%s\n",i,node->name,node->value);
	}
	fprintf(stderr,"\t}\n");
    }
}

static void
dumphrb(hrb_t* hrb)
{
    fprintf(stderr,"hrb={\n");
    if(hrb != NULL) {
	fprintf(stderr,"\tresource=%s\n",hrb->resource);
	fprintf(stderr,"\tversion=%s\n",hrb->version);
	fprintf(stderr,"\tbody=|%.*s|\n",(int)ncbyteslength(hrb->body),ncbytescontents(hrb->body));
	dumphrbnodes(hrb->headers);
    }
    fprintf(stderr,"}\n");

}
#endif

static void
hrb_node_free(hrb_node_t *node)
{
    if(node != NULL) {
	nullfree(node->name);
	nullfree(node->value);
	nullfree(node->cat);
	nullfree(node->lowername);
	free(node);
    }
}

/****************************************************************************
 * S3R FUNCTIONS
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_close()
 * Purpose:
 *     Close communications through given S3 Request Handle (`s3r_t`)
 *     and clean up associated resources.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 *         - fails if handle is null or has invalid magic number
 * Programmer: Jacob Smith
 *             2017-08-31
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_close(s3r_t *handle)
{
    int ret_value = SUCCEED;

    TRACE(0,"handle=%p",handle);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_close.\n");
#endif

    if (handle == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle cannot be null.");
    if (handle->magic != S3COMMS_S3R_MAGIC)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle has invalid magic.");

    if(handle->curlheaders != NULL) {
        curl_slist_free_all(handle->curlheaders);
        handle->curlheaders = NULL;
    }
    curl_easy_cleanup(handle->curlhandle);

    nullfree(handle->rootpath);
    nullfree(handle->region);
    nullfree(handle->accessid);
    nullfree(handle->accesskey);
    nullfree(handle->reply);
    nullfree(handle->signing_key);
    free(handle);

done:
    return UNTRACE(ret_value);
} /* NCH5_s3comms_s3r_close */


/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_getsize()
 * Purpose:
 *    Get the number of bytes of handle's target resource.
 *    Sets handle and curlhandle with to enact an HTTP HEAD request on file,
 *    and parses received headers to extract "Content-Length" from response
 *    headers, storing file size at `handle->filesize`.
 *    Critical step in opening (initiating) an `s3r_t` handle.
 *    Wraps `s3r_read()`.
 *    Sets curlhandle to write headers to a temporary buffer (using extant
 *    write callback) and provides no buffer for body.
 *    Upon exit, unsets HTTP HEAD settings from curl handle, returning to
 *    initial state. In event of error, curl handle state is undefined and is
 *    not to be trusted.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 * Programmer: Jacob Smith
 *             2017-08-23
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_getsize(s3r_t *handle, const char* url, long long* sizep)
{
    int ret_value      = SUCCEED;
    char* contentlength = NULL;
    char* value = NULL;
    long long content_length = -1;
    long httpcode = 0;

    TRACE(0,"handle=%p url=%s sizep=%p",handle,url,sizep);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_getsize.\n");
#endif

    if((ret_value = NCH5_s3comms_s3r_head(handle, url, "Content-Length", NULL, &httpcode, &contentlength)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "NCH5_s3comms_s3r_head failed.");

    if((ret_value = httptonc(httpcode))) goto done;

    /******************
     * PARSE RESPONSE *
     ******************/

    value = strchr(contentlength,':');
    if(value == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not find content length value");
    value++;
    content_length = strtoumax(value, NULL, 0);
    if (UINTMAX_MAX > SIZE_MAX && content_length > SIZE_MAX)
        HGOTO_ERROR(H5E_ARGS, NC_ERANGE, FAIL, "content_length overflows size_t");

    if (errno == ERANGE) /* errno set by strtoumax*/
        HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL,
                    "could not convert found \"Content-Length\" response (\"%s\")",
                    contentlength); /* range is null-terminated, remember */

    if(sizep) {*sizep = (long long)content_length;}

done:
    nullfree(contentlength);
    return UNTRACEX(ret_value,"size=%lld",(sizep?-1:*sizep));
} /* NCH5_s3comms_s3r_getsize */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_deletekey()
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 * Programmer: Dennis Heimbigner
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_deletekey(s3r_t *handle, const char* url, long* httpcodep)
{
    int ret_value      = SUCCEED;
    VString* data      = vsnew();
    long httpcode      = 0;

    TRACE(0,"handle=%p url=%s httpcodep=%p",handle,url,httpcodep);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_deletekey.\n");
#endif

    /*********************
     * Execute           *
     *********************/

    if((ret_value = NCH5_s3comms_s3r_execute(handle, url, HTTPDELETE, NULL, NULL, NULL, &httpcode, data)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "execute failed.");

    /******************
     * RESPONSE *
     ******************/
    if((ret_value = httptonc(httpcode))) goto done;
    if(httpcode != 204) 
        HGOTO_ERROR(H5E_ARGS, NC_ECANTREMOVE, FAIL, "deletekey failed.");     

done:
    vsfree(data);
    if(httpcodep) *httpcodep = httpcode;
    return UNTRACEX(ret_value,"httpcode=%d",INULL(httpcodep));
} /* NCH5_s3comms_s3r_getsize */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_head()
 * Purpose:
 *    Generic HEAD request
 * @param
 * @return NC_NOERR  if exists
 * @return NC_EINVAL if not exits
 * @return error     otherwise
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_head(s3r_t *handle, const char* url, const char* header, const char* query, long* httpcodep, char** valuep)
{
    int ret_value = SUCCEED;
    VString* data = vsnew();
    long httpcode = 0;
    
    TRACE(0,"handle=%p url=%s header=%s query=%s httpcodep=%p valuep=%p",handle,url,SNULL(header),SNULL(query),httpcodep,valuep);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_head.\n");
#endif

    if (url == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle has bad (null) url.");

    if((ret_value = validate_handle(handle,url)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "invalid handle.");

    /*******************
     * PERFORM REQUEST *
     *******************/

     /* only http metadata will be sent by server and recorded by s3comms
     */
    if (SUCCEED != NCH5_s3comms_s3r_execute(handle, url, HTTPHEAD, NULL, header, NULL, &httpcode, data))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "problem in reading during getsize.");

    if((ret_value = httptonc(httpcode))) goto done;

    if(header != NULL) {
        if(vslength(data) == 0)
            HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "HTTP metadata: header=%s; not found",header);
        else if (vslength(data) > CURL_MAX_HTTP_HEADER)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "HTTP metadata buffer overrun");
#if S3COMMS_DEBUG
        else
           fprintf(stderr, "HEAD: OK\n");
#endif
    }

    /******************
     * PARSE RESPONSE *
     ******************/

    if(header != NULL) {
        char* content;
	content = vsextract(data);
        if(valuep) {*valuep = content;}
    }

    /**********************
     * UNDO HEAD SETTINGS *
     **********************/

    if((ret_value = curl_reset(handle)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "error while re-setting CURL options.");

done:
    if(httpcodep) *httpcodep = httpcode;
    vsfree(data);
    return UNTRACEX(ret_value,"httpcodep=%d",INULL(httpcodep));
} /* NCH5_s3comms_s3r_getsize */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_execute()
 * Purpose:
 *     Execute an HTTP verb and optionally return the response.
 *     Uses configured "curl easy handle" to perform request.
 *     In event of error, buffer should remain unaltered.
 *     If handle is set to authorize a request, creates a new (temporary)
 *     HTTP Request object (hrb_t) for generating requisite headers,
 *     which is then translated to a `curl slist` and set in the curl handle
 *     for the request.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 * Programmer: Dennis Heimbigner
 *----------------------------------------------------------------------------
 */
/* TODO: Need to simplify this signature; it is too long */
static int
NCH5_s3comms_s3r_execute(s3r_t *handle, const char* url,
			 HTTPVerb verb,
			 const char* range,
			 const char* searchheader,
		         const char** otherheaders,
			 long* httpcodep,
			 VString* data)
{
    int    ret_value = SUCCEED;
    NCURI* purl= NULL;
    struct s3r_cbstruct sds = {S3COMMS_CALLBACK_STRUCT_MAGIC, NULL, NULL, 0};
    long httpcode = 0;

#ifdef DEBUG
    printf(">>> NCH5_s3comms_s3r_execute(url=%s verb=%s range=%s searchheader=%s)\n",url,verbtext(verb),SNULL(range),SNULL(searchheader));
    fflush(stdout);
#endif

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_execute.\n");
#endif

    /**************************************
     * ABSOLUTELY NECESSARY SANITY-CHECKS *
     **************************************/

    if((ret_value = validate_handle(handle, url)))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "invalid handle.");

    ncuriparse(url,&purl);
    if((ret_value = validate_url(purl)))
       HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "unparseable url: %s", url);

    /*********************
     * Setup             *
     *********************/

    sds.data = data;
    if (verb == HTTPHEAD)
	sds.key = searchheader;

    /*******************
     * COMPILE REQUEST *
     *******************/

    if((ret_value = build_request(handle,purl,range,otherheaders,data,verb)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "unable to build request.");

    /*********************
     * PREPARE CURL
     *********************/

    if((ret_value = request_setup(handle, url, verb, &sds)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "read_request_setup failed.");

    /*******************
     * PERFORM REQUEST *
     *******************/

    if((ret_value = perform_request(handle,&httpcode)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "unable perform request.");

done:
    if(httpcodep) *httpcodep = httpcode;
    ncurifree(purl);
    /* clean any malloc'd resources */
    curl_reset(handle);
    return (ret_value);;
} /* NCH5_s3comms_s3r_read */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_open()
 * Purpose:
 *     Logically 'open' a file hosted on S3.
 *     - create new Request Handle
 *     - copy supplied url
 *     - copy authentication info if supplied
 *     - create CURL handle
 *     - fetch size of file
 *         - connect with server and execute HEAD request
 *     - return request handle ready for reads
 *     To use 'default' port to connect, `port` should be 0.
 *     To prevent AWS4 authentication, pass null pointer to `region`, `id`,
 *     and `signing_key`.
 *     Uses `NCH5_s3comms_parse_url()` to validate and parse url input.
 * Return:
 *     - SUCCESS: Pointer to new request handle.
 *     - FAILURE: NULL
 *         - occurs if:
 *             - authentication strings are inconsistent
 *             - must _all_ be null, or have at least `region` and `id`
 *             - url is NULL (no filename)
 *             - unable to parse url (malformed?)
 *             - error while performing `getsize()`
 * Programmer: Jacob Smith
 *             2017-09-01
 *----------------------------------------------------------------------------
 */
s3r_t *
NCH5_s3comms_s3r_open(const char* root, NCS3SVC svc, const char *region, const char *access_id, const char* access_key)
{
    int ret_value = SUCCEED;
    size_t         tmplen    = 0;
    CURL          *curlh     = NULL;
    s3r_t         *handle    = NULL;
    unsigned char *signing_key = NULL;
    char           iso8601now[ISO8601_SIZE];
    struct tm     *now           = NULL;
    const char* signingregion = AWS_GLOBAL_DEFAULT_REGION;

    TRACE(0,"root=%s region=%s access_id=%s access_key=%s",root,region,access_id,access_key);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_open.\n");
#endif

    /* setup */
    iso8601now[0] = '\0';

    handle = (s3r_t *)calloc(1,sizeof(s3r_t));
    if (handle == NULL)
	HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "could not malloc space for handle.");

    handle->magic	= S3COMMS_S3R_MAGIC;

    /*************************************
     * RECORD THE ROOT PATH
     *************************************/

    switch (svc) {
    case NCS3:
        /* Verify that the region is a substring of root */
        if(region != NULL && region[0] != '\0') {
	    if(strstr(root,region) == NULL)
	        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "region not present in root path.");
        }
	break;
    default: break;
    }
    handle->rootpath = nulldup(root);

    /*************************************
     * RECORD AUTHENTICATION INFORMATION *
     *************************************/

    /* copy strings */
    if(nulllen(region) != 0) {
        tmplen = nulllen(region) + 1;
        handle->region = (char *)malloc(sizeof(char) * tmplen);
        if (handle->region == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "could not malloc space for handle region copy.");
        memcpy(handle->region, region, tmplen);
    }

    if(nulllen(access_id) != 0) {
        tmplen = nulllen(access_id) + 1;
        handle->accessid = (char *)malloc(sizeof(char) * tmplen);
        if (handle->accessid == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "could not malloc space for handle ID copy.");
        memcpy(handle->accessid, access_id, tmplen);
    }
    
    if(nulllen(access_key) != 0) {
        tmplen = nulllen(access_key) + 1;
        handle->accesskey = (char *)malloc(sizeof(char) * tmplen);
        if (handle->accesskey == NULL)
           HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "could not malloc space for handle access key copy.");
        memcpy(handle->accesskey, access_key, tmplen);
    }

    now = gmnow();
    if (ISO8601NOW(iso8601now, now) != (ISO8601_SIZE - 1))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "unable to get current time");
    memcpy(handle->iso8601now,iso8601now,ISO8601_SIZE);

    /* Do optional authentication */
    if(access_id != NULL && access_key != NULL) { /* We are authenticating */
        /* Need several pieces of info for authentication */
        if (nulllen(handle->region) > 0)
	     signingregion = region;
//            HGOTO_ERROR(H5E_ARGS, NC_EAUTH, NULL, "region cannot be null.");
        if (nulllen(handle->accessid)==0)
            HGOTO_ERROR(H5E_ARGS, NC_EAUTH, NULL, "access id cannot be null.");
        if (nulllen(handle->accesskey)==0)
            HGOTO_ERROR(H5E_ARGS, NC_EAUTH, NULL, "signing key cannot be null.");

        /* Compute the signing key */
        if (SUCCEED != NCH5_s3comms_signing_key(&signing_key, access_key, signingregion, iso8601now))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "problem in NCH5_s3comms_s3comms_signing_key.");
        if (signing_key == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EAUTH, NULL, "signing key cannot be null.");
	handle->signing_key = signing_key;
	signing_key = NULL;

    } /* if authentication information provided */

    /************************
     * INITIATE CURL HANDLE *
     ************************/

    curlh = curl_easy_init();
    if (curlh == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "problem creating curl easy handle!");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_HTTP_VERSION).");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_FAILONERROR, 1L))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_FAILONERROR).");

    handle->curlhandle = curlh;

    /*********************
     * FINAL PREPARATION *
     *********************/

    assert(handle->httpverb != NULL);
    strcpy(handle->httpverb, "GET");

done:
    nullfree(signing_key);
    if (ret_value != SUCCEED) {
        if (curlh != NULL)
            curl_easy_cleanup(curlh);
        if (handle != NULL) {
            if(handle->region != NULL) free(handle->region);
            if(handle->accessid != NULL) free(handle->accessid);
            if(handle->accesskey != NULL) free(handle->accesskey);
            if(handle->rootpath != NULL) free(handle->rootpath);
            free(handle);
            handle = NULL;
        }
    }

    (void)UNTRACEX(ret_value,"handle=%p",handle);
    return handle;
} /* NCH5_s3comms_s3r_open */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_read()
 * Purpose:
 *     Read file pointed to by request handle.
 *     Optionally specify byterange of `offset` .. `offset + len` bytes to buffer `dest`.
 *     In event of error, buffer should remain unaltered.
 *     If handle is set to authorize a request, creates a new (temporary)
 *     HTTP Request object (hrb_t) for generating requisite headers,
 *     which is then translated to a `curl slist` and set in the curl handle
 *     for the request.
 *     `dest` _may_ be NULL, but no body data will be recorded.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 * Programmer: Jacob Smith
 *             2017-08-22
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_read(s3r_t *handle, const char* url, size_t offset, size_t len, s3r_buf_t* dest)
{
    char              *rangebytesstr = NULL;
    int                ret_value = SUCCEED;
    long               httpcode;
    VString           *wrap = vsnew();

    TRACE(0,"handle=%p url=%s offset=%ld len=%ld, dest=%p",handle,url,(long)offset,(long)len,dest);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_read.\n");
#endif

    /*********************
     * FORMAT HTTP RANGE *
     *********************/

    if((ret_value = build_range(offset,len,&rangebytesstr)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "build_range failed.");

    /*********************
     * Execute           *
     *********************/

    vssetcontents(wrap,dest->content,dest->count);
    vssetlength(wrap,0);

    if((ret_value = NCH5_s3comms_s3r_execute(handle, url, HTTPGET, rangebytesstr, NULL, NULL, &httpcode, wrap)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "execute failed.");
    if((ret_value = httptonc(httpcode))) goto done;

done:
    (void)vsextract(wrap);
    vsfree(wrap);
    /* clean any malloc'd resources */
    nullfree(rangebytesstr);
    curl_reset(handle);
    return UNTRACE(ret_value);;
} /* NCH5_s3comms_s3r_read */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_write()
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 * Programmer: Dennis Heimbigner
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_write(s3r_t *handle, const char* url, const s3r_buf_t* data)
{
    int ret_value = SUCCEED;
    VList* otherheaders = vlistnew();
    char digits[64];
    long httpcode = 0;
    VString* wrap = vsnew();

    TRACE(0,"handle=%p url=%s |data|=%d",handle,url,data->count);

    snprintf(digits,sizeof(digits),"%llu",(unsigned long long)data->count);

    vlistpush(otherheaders,strdup("Content-Length"));
    vlistpush(otherheaders,strdup(digits));
    vlistpush(otherheaders,strdup("Content-Type"));
    vlistpush(otherheaders,strdup("binary/octet-stream"));
    vlistpush(otherheaders,NULL);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_write.\n");
#endif

    /*********************
     * Execute           *
     *********************/

    vssetcontents(wrap,data->content,data->count);
    vssetlength(wrap,data->count);
    if((ret_value = NCH5_s3comms_s3r_execute(handle, url, HTTPPUT, NULL, NULL, (const char**)vlistcontents(otherheaders), &httpcode, wrap)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "execute failed.");
    if((ret_value = httptonc(httpcode))) goto done;
    
done:
    (void)vsextract(wrap);
    vsfree(wrap);
    /* clean any malloc'd resources */
    vlistfreeall(otherheaders);
    curl_reset(handle);
    return UNTRACE(ret_value);
} /* NCH5_s3comms_s3r_write */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_s3r_getkeys()
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 * Programmer: Dennis Heimbigner
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_s3r_getkeys(s3r_t *handle, const char* url, s3r_buf_t* response)
{
    int ret_value = SUCCEED;
    const char* otherheaders[3] = {"Content-Type", "application/xml", NULL};
    long httpcode = 0;
    VString* content = vsnew();

    TRACE(0,"handle=%p url=%s response=%p",handle,url,response->content);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_s3r_getkeys.\n");
#endif

    /*********************
     * Execute           *
     *********************/

    if((SUCCEED != NCH5_s3comms_s3r_execute(handle, url, HTTPGET, NULL, NULL, otherheaders, &httpcode, content)))
        HGOTO_ERROR(H5E_ARGS, ret_value, FAIL, "execute failed.");
    if((ret_value = httptonc(httpcode))) goto done;
    if(response) {
	response->count = vslength(content);
	response->content = vsextract(content);
    }

done:
    vsfree(content);
    /* clean any malloc'd resources */
    curl_reset(handle);
    return UNTRACEX(ret_value,"response=[%d]",ncbyteslength(response));
} /* NCH5_s3comms_s3r_getkeys */

/****************************************************************************
 * MISCELLANEOUS FUNCTIONS
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * Function: gmnow()
 * Purpose:
 *    Get the output of `time.h`'s `gmtime()` call while minimizing setup
 *    clutter where important.
 * Return:
 *    Pointer to resulting `struct tm`,as created by gmtime(time_t * T).
 * Programmer: Jacob Smith
 *             2017-07-12
 *----------------------------------------------------------------------------
 */
struct tm *
gmnow(void)
{
    time_t     now;
    time_t    *now_ptr   = &now;
    struct tm *ret_value = NULL;

    /* Doctor assert, checks against error in time() */
    if ((time_t)(-1) != time(now_ptr))
        ret_value = gmtime(now_ptr);

    assert(ret_value != NULL);

    return ret_value;
} /* end gmnow() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_aws_canonical_request()
 * Purpose:
 *     Compose AWS "Canonical Request" (and signed headers string)
 *     as defined in the REST API documentation.
 *     Both destination strings are null-terminated.
 *     Destination string arguments must be provided with adequate space.
 *     Canonical Request format:
 *      <HTTP VERB>"\n"
 *      <resource path>"\n"
 *      <query string>"\n"
 *      <header1>"\n" (`lowercase(name)`":"`trim(value)`)
 *      <header2>"\n"
 *      ... (headers sorted by name)
 *      <header_n>"\n"
 *      "\n"
 *      <signed headers>"\n" (`lowercase(header 1 name)`";"`header 2 name`;...)
 *      <hex-string of sha256sum of body> ("e3b0c4429...", e.g.)
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - writes canonical request to respective `...dest` strings
 *     - FAILURE: `FAIL`
 *         - one or more input argument was NULL
 *         - internal error
 * Programmer: Jacob Smith
 *             2017-10-04
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_aws_canonical_request(VString* canonical_request_dest, VString* signed_headers_dest,
                                   HTTPVerb verb,
                                   const char* query,
                                   const char* payloadsha256,
                                   hrb_t *http_request)
{
    hrb_node_t *node         = NULL;
    int      ret_value    = SUCCEED;
    int i;
    
    const char* sverb = verbtext(verb);
    const char* query_params = (query?query:"");

    /* "query params" refers to the optional element in the URL, e.g.
     *     http://bucket.aws.com/myfile.txt?max-keys=2&prefix=J
     *                                      ^-----------------^
     * Not handled/implemented as of 2017-10-xx.
     * Element introduced as empty placeholder and reminder.
     * Further research to be done if this is ever relevant for the
     * VFD use-cases.
     */

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_aws_canonical_request.\n");
#endif

    if (http_request == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "hrb object cannot be null.");
    assert(http_request->magic == S3COMMS_HRB_MAGIC);

    if (canonical_request_dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "canonical request destination cannot be null.");

    if (signed_headers_dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "signed headers destination cannot be null.");

    /* HTTP verb, resource path, and query string lines */
    vscat(canonical_request_dest,sverb);
    vscat(canonical_request_dest,"\n");
    vscat(canonical_request_dest,http_request->resource);
    vscat(canonical_request_dest,"\n");
    vscat(canonical_request_dest,query_params);
    vscat(canonical_request_dest,"\n");
    
    /* write in canonical headers, building signed headers concurrently */
    for(i=0;i<vlistlength(http_request->headers);i++) {
        node = (hrb_node_t*)vlistget(http_request->headers,i); /* assumed sorted */
        if(i>0) vscat(signed_headers_dest,";");
        assert(node->magic == S3COMMS_HRB_NODE_MAGIC);
        vscat(canonical_request_dest,node->lowername);
        vscat(canonical_request_dest,":");
        vscat(canonical_request_dest,node->value);
        vscat(canonical_request_dest,"\n");
        vscat(signed_headers_dest,node->lowername);
    } /* end while node is not NULL */

    /* append signed headers and payload hash
     * NOTE: at present, no HTTP body is handled, per the nature of
     *       requests/range-gets
     */
    vscat(canonical_request_dest, "\n");
    vscat(canonical_request_dest, vscontents(signed_headers_dest));
    vscat(canonical_request_dest, "\n");
    vscat(canonical_request_dest, payloadsha256);

done:
    return (ret_value);
} /* end NCH5_s3comms_aws_canonical_request() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_bytes_to_hex()
 * Purpose:
 *     Produce human-readable hex string [0-9A-F] from sequence of bytes.
 *     For each byte (char), writes two-character hexadecimal representation.
 *     No null-terminator appended.
 *     Assumes `dest` is allocated to enough size (msg_len * 2).
 *     Fails if either `dest` or `msg` are null.
 *     `msg_len` message length of 0 has no effect.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - hex string written to `dest` (not null-terminated)
 *     - FAILURE: `FAIL`
 *         - `dest == NULL`
 *         - `msg == NULL`
 * Programmer: Jacob Smith
 *             2017-07-12
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_bytes_to_hex(char *dest, const unsigned char *msg, size_t msg_len, int lowercase)
{
    size_t i         = 0;
    int ret_value = SUCCEED;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_bytes_to_hex.\n");
#endif

    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "hex destination cannot be null.");
    if (msg == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "bytes sequence cannot be null.");

    for (i = 0; i < msg_len; i++) {
        int chars_written = snprintf(&(dest[i * 2]), 3, /* 'X', 'X', '\n' */
                                       (lowercase == TRUE) ? "%02x" : "%02X", msg[i]);
        if (chars_written != 2)
            HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "problem while writing hex chars for %c", msg[i]);
    }

done:
    return (ret_value);
} /* end NCH5_s3comms_bytes_to_hex() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_HMAC_SHA256()
 * Purpose:
 *     Generate Hash-based Message Authentication Checksum using the SHA-256
 *     hashing algorithm.
 *     Given a key, message, and respective lengths (to accommodate null
 *     characters in either), generate _hex string_ of authentication checksum
 *     and write to `dest`.
 *     `dest` must be at least `SHA256_DIGEST_LENGTH * 2` characters in size.
 *     Not enforceable by this function.
 *     `dest` will _not_ be null-terminated by this function.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - hex string written to `dest` (not null-terminated)
 *     - FAILURE: `FAIL`
 *         - `dest == NULL`
 *         - error while generating hex string output
 * Programmer: Jacob Smith
 *             2017-07-??
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_HMAC_SHA256(const unsigned char *key, size_t key_len, const char *msg, size_t msg_len,
                         char *dest)
{
    unsigned char md[SHA256_DIGEST_LENGTH];
    unsigned int  md_len    = SHA256_DIGEST_LENGTH;
    int        ret_value = SUCCEED;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_HMAC_SHA256.\n");
#endif

    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "destination cannot be null.");

#if 0
    HMAC(EVP_sha256(), key, (int)key_len, (const unsigned char *)msg, msg_len, md, &md_len);
#else
    if(CURLE_OK != Curl_hmacit(Curl_HMAC_SHA256,
                     key, key_len,
                     msg, msg_len,
                     md))
        HGOTO_ERROR(H5E_ARGS, NC_EINTERNAL, FAIL, "Curl_hmacit failure.");
#endif

    if (NCH5_s3comms_bytes_to_hex(dest, (const unsigned char *)md, (size_t)md_len, TRUE) != SUCCEED)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not convert to hex string.");

done:
    return (ret_value);
} /* NCH5_s3comms_HMAC_SHA256 */

/*-----------------------------------------------------------------------------
 * Function: H5FD__s3comms_load_aws_creds_from_file()
 * Purpose:
 *     Extract AWS configuration information from a target file.
 *     Given a file and a profile name, e.g. "ros3_vfd_test", attempt to locate
 *     that region in the file. If not found, returns in error and output
 *     pointers are not modified.
 *     If the profile label is found, attempts to locate and parse configuration
 *     data, stopping at the first line where:
 *     + reached end of file
 *     + line does not start with a recognized setting name
 *     Following AWS documentation, looks for any of:
 *     + aws_access_key_id
 *     + aws_secret_access_key
 *     + region
 *     To be valid, the setting must begin the line with one of the keywords,
 *     followed immediately by an equals sign '=', and have some data before
 *     newline at end of line.
 *     + `spam=eggs` would be INVALID because name is unrecognized
 *     + `region = us-east-2` would be INVALID because of spaces
 *     + `region=` would be INVALID because no data.
 *     Upon successful parsing of a setting line, will store the result in the
 *     corresponding output pointer. If the output pointer is NULL, will skip
 *     any matching setting line while parsing -- useful to prevent overwrite
 *     when reading from multiple files.
 * Return:
 *     + SUCCESS: `SUCCEED`
 *         + no error. settings may or may not have been loaded.
 *     + FAILURE: `FAIL`
 *         + internal error occurred.
 *         + -1 :: unable to format profile label
 *         + -2 :: profile name/label not found in file
 *         + -3 :: some other error
 * Programmer: Jacob Smith
 *             2018-02-27
 *-----------------------------------------------------------------------------
 */
static int
H5FD__s3comms_load_aws_creds_from_file(FILE *file, const char *profile_name, char *key_id, char *access_key,
                                       char *aws_region)
{
    char        profile_line[32];
    char        buffer[128];
    const char *setting_names[] = {
        "region",
        "aws_access_key_id",
        "aws_secret_access_key",
    };
    char *const setting_pointers[] = {
        aws_region,
        key_id,
        access_key,
    };
    unsigned setting_count = 3;
    int   ret_value     = SUCCEED;
    unsigned buffer_i      = 0;
    unsigned setting_i     = 0;
    int      found_setting = 0;
    char    *line_buffer   = &(buffer[0]);

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called load_aws_creds_from_file.\n");
#endif

    /* format target line for start of profile */
    if (32 < snprintf(profile_line, 32, "[%s]", profile_name))
        HGOTO_ERROR(H5E_ARGS, NC_EINTERNAL, FAIL, "unable to format profile label");

    /* look for start of profile */
    do {
        /* clear buffer */
        for (buffer_i = 0; buffer_i < 128; buffer_i++)
            buffer[buffer_i] = 0;

        line_buffer = fgets(line_buffer, 128, file);
        if (line_buffer == NULL) /* reached end of file */
            goto done;
    } while (strncmp(line_buffer, profile_line, nulllen(profile_line)));

    /* extract credentials from lines */
    do {
        /* clear buffer */
        for (buffer_i = 0; buffer_i < 128; buffer_i++)
            buffer[buffer_i] = 0;

        /* collect a line from file */
        line_buffer = fgets(line_buffer, 128, file);
        if (line_buffer == NULL)
            goto done; /* end of file */

        /* loop over names to see if line looks like assignment */
        for (setting_i = 0; setting_i < setting_count; setting_i++) {
            size_t      setting_name_len = 0;
            const char *setting_name     = NULL;
            char        line_prefix[128];

            setting_name     = setting_names[setting_i];
            setting_name_len = nulllen(setting_name);
            if (snprintf(line_prefix, 128, "%s=", setting_name) < 0)
                HGOTO_ERROR(H5E_ARGS, NC_EINTERNAL, FAIL, "unable to format line prefix");

            /* found a matching name? */
            if (!strncmp(line_buffer, line_prefix, setting_name_len + 1)) {
                found_setting = 1;

                /* skip NULL destination buffer */
                if (setting_pointers[setting_i] == NULL)
                    break;

                /* advance to end of name in string */
                do {
                    line_buffer++;
                } while (*line_buffer != 0 && *line_buffer != '=');

                if (*line_buffer == 0 || *(line_buffer + 1) == 0)
                    HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "incomplete assignment in file");
                line_buffer++; /* was pointing at '='; advance */

                /* copy line buffer into out pointer */
                strncpy(setting_pointers[setting_i], (const char *)line_buffer, nulllen(line_buffer));

                /* "trim" tailing whitespace by replacing with null terminator*/
                buffer_i = 0;
                while (!isspace(setting_pointers[setting_i][buffer_i]))
                    buffer_i++;
                setting_pointers[setting_i][buffer_i] = '\0';

                break; /* have read setting; don't compare with others */
            }          /* end if possible name match */
        }              /* end for each setting name */
    } while (found_setting);

done:
    return (ret_value);
} /* end H5FD__s3comms_load_aws_creds_from_file() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_load_aws_profile()
 * Purpose :
 *     Read aws profile elements from standard location on system and store
 *     settings in memory.
 *     Looks for both `~/.aws/config` and `~/.aws/credentials`, the standard
 *     files for AWS tools. If a file exists (can be opened), looks for the
 *     given profile name and reads the settings into the relevant buffer.
 *     Any setting duplicated in both files will be set to that from
 *     `credentials`.
 *     Settings are stored in the supplied buffers as null-terminated strings.
 * Return:
 *     + SUCCESS: `SUCCEED` (0)
 *         + no error occurred and all settings were populated
 *     + FAILURE: `FAIL` (-1)
 *         + internal error occurred
 *         + unable to locate profile
 *         + region, key id, and secret key were not all found and set
 * Programmer: Jacob Smith
 *             2018-02-27
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_load_aws_profile(const char *profile_name, char *key_id_out, char *secret_access_key_out,
                              char *aws_region_out)
{
    int ret_value = SUCCEED;
    FILE  *credfile  = NULL;
    char   awspath[117];
    char   filepath[128];
    int    ret = 0;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_load_aws_profile.\n");
#endif

#ifdef H5_HAVE_WIN32_API
    ret = snprintf(awspath, 117, "%s/.aws/", getenv("USERPROFILE"));
#else
    ret = snprintf(awspath, 117, "%s/.aws/", getenv("HOME"));
#endif
    if (ret < 0 || (size_t)ret >= 117)
        HGOTO_ERROR(H5E_ARGS, NC_EINTERNAL, FAIL, "unable to format home-aws path");
    ret = snprintf(filepath, 128, "%s%s", awspath, "credentials");
    if (ret < 0 || (size_t)ret >= 128)
        HGOTO_ERROR(H5E_ARGS, NC_EINTERNAL, FAIL, "unable to format credentials path");

    credfile = fopen(filepath, "r");
    if (credfile != NULL) {
        if (H5FD__s3comms_load_aws_creds_from_file(credfile, profile_name, key_id_out, secret_access_key_out,
                                                   aws_region_out) != SUCCEED)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to load from aws credentials");
        if (fclose(credfile) == EOF)
            HGOTO_ERROR(H5E_FILE, NC_EACCESS, FAIL, "unable to close credentials file");
        credfile = NULL;
    } /* end if credential file opened */

    ret = snprintf(filepath, 128, "%s%s", awspath, "config");
    if (ret < 0 || (size_t)ret >= 128)
        HGOTO_ERROR(H5E_ARGS, NC_EINTERNAL, FAIL, "unable to format config path");
    credfile = fopen(filepath, "r");
    if (credfile != NULL) {
        if (H5FD__s3comms_load_aws_creds_from_file(
                credfile, profile_name, (*key_id_out == 0) ? key_id_out : NULL,
                (*secret_access_key_out == 0) ? secret_access_key_out : NULL,
                (*aws_region_out == 0) ? aws_region_out : NULL) != SUCCEED)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to load from aws config");
        if (fclose(credfile) == EOF)
            HGOTO_ERROR(H5E_FILE, NC_EACCESS, FAIL, "unable to close config file");
        credfile = NULL;
    } /* end if credential file opened */

    /* fail if not all three settings were loaded */
    if (*key_id_out == 0 || *secret_access_key_out == 0 || *aws_region_out == 0)
        ret_value = NC_EINVAL;

done:
    if (credfile != NULL)
        if (fclose(credfile) == EOF)
            HDONE_ERROR(H5E_ARGS, NC_EACCESS, FAIL, "problem error-closing aws configuration file");

    return (ret_value);
} /* end NCH5_s3comms_load_aws_profile() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_nlowercase()
 * Purpose:
 *     From string starting at `s`, write `len` characters to `dest`,
 *     converting all to lowercase.
 *     Behavior is undefined if `s` is NULL or `len` overruns the allocated
 *     space of either `s` or `dest`.
 *     Provided as convenience.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - upon completion, `dest` is populated
 *     - FAILURE: `FAIL`
 *         - `dest == NULL`
 * Programmer: Jacob Smith
 *             2017-09-18
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_nlowercase(char *dest, const char *s, size_t len)
{
    int ret_value = SUCCEED;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_nlowercase.\n");
#endif

    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "destination cannot be null.");

    if (len > 0) {
        memcpy(dest, s, len);
        do {
            len--;
            dest[len] = (char)tolower((int)dest[len]);
        } while (len > 0);
    }

done:
    return (ret_value);
} /* end NCH5_s3comms_nlowercase() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_percent_encode_char()
 * Purpose:
 *     "Percent-encode" utf-8 character `c`, e.g.,
 *         '$' -> "%24"
 *         '¢' -> "%C2%A2"
 *     `c` cannot be null.
 *     Does not (currently) accept multi-byte characters...
 *     limit to (?) u+00ff, well below upper bound for two-byte utf-8 encoding
 *        (u+0080..u+07ff).
 *     Writes output to `repr`.
 *     `repr` cannot be null.
 *     Assumes adequate space i `repr`...
 *         >>> char[4] or [7] for most characters,
 *         >>> [13] as theoretical maximum.
 *     Representation `repr` is null-terminated.
 *     Stores length of representation (without null terminator) at pointer
 *     `repr_len`.
 * Return : SUCCEED/FAIL
 *     - SUCCESS: `SUCCEED`
 *         - percent-encoded representation  written to `repr`
 *         - 'repr' is null-terminated
 *     - FAILURE: `FAIL`
 *         - `c` or `repr` was NULL
 * Programmer: Jacob Smith
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_percent_encode_char(char *repr, const unsigned char c, size_t *repr_len)
{
    unsigned int i             = 0;
    int          chars_written = 0;
    int       ret_value     = SUCCEED;
#if S3COMMS_DEBUG
    unsigned char s[2]   = {c, 0};
    unsigned char hex[3] = {0, 0, 0};
#endif

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_percent_encode_char.\n");
#endif

    if (repr == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "no destination `repr`.");

#if S3COMMS_DEBUG
    NCH5_s3comms_bytes_to_hex((char *)hex, s, 1, FALSE);
    fprintf(stdout, "    CHAR: \'%s\'\n", s);
    fprintf(stdout, "    CHAR-HEX: \"%s\"\n", hex);
#endif

    if (c <= (unsigned char)0x7f) {
        /* character represented in a single "byte"
         * and single percent-code
         */
#if S3COMMS_DEBUG
        fprintf(stdout, "    SINGLE-BYTE\n");
#endif
        *repr_len     = 3;
        chars_written = snprintf(repr, 4, "%%%02X", c);
        if (chars_written < 0)
            HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "cannot write char %c", c);
    } /* end if single-byte unicode char */
    else {
        /* multi-byte, multi-percent representation
         */
        unsigned int  acc        = 0; /* byte accumulator */
        unsigned int  k          = 0; /* uint character representation */
        unsigned int  stack_size = 0;
        unsigned char stack[4]   = {0, 0, 0, 0};
#if S3COMMS_DEBUG
        fprintf(stdout, "    MULTI-BYTE\n");
#endif
        stack_size = 0;
        k          = (unsigned int)c;
        *repr_len  = 0;
        do {
            /* push number onto stack in six-bit slices
             */
            acc = k;
            acc >>= 6; /* cull least */
            acc <<= 6; /* six bits   */
            stack[stack_size++] = (unsigned char)(k - acc);
            k                   = acc >> 6;
        } while (k > 0);

        /* `stack` now has two to four six-bit 'numbers' to be put into
         * UTF-8 byte fields.
         */

#if S3COMMS_DEBUG
        fprintf(stdout, "    STACK:\n    {\n");
        for (i = 0; i < stack_size; i++) {
            NCH5_s3comms_bytes_to_hex((char *)hex, (&stack[i]), 1, FALSE);
            hex[2] = 0;
            fprintf(stdout, "      %s,\n", hex);
        }
        fprintf(stdout, "    }\n");
#endif

        /****************
         * leading byte *
         ****************/

        /* prepend 11[1[1]]0 to first byte */
        /* 110xxxxx, 1110xxxx, or 11110xxx */
        acc = 0xC0;                         /* 0x11000000 */
        acc += (stack_size > 2) ? 0x20 : 0; /* 0x00100000 */
        acc += (stack_size > 3) ? 0x10 : 0; /* 0x00010000 */
        stack_size--;
        chars_written = snprintf(repr, 4, "%%%02X", (unsigned char)(acc + stack[stack_size]));
        if (chars_written < 0)
            HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "cannot write char %c", c);
        *repr_len += 3;

        /************************
         * continuation byte(s) *
         ************************/

        /* 10xxxxxx */
        for (i = 0; i < stack_size; i++) {
            chars_written =
                snprintf(&repr[i * 3 + 3], 4, "%%%02X", (unsigned char)(0x80 + stack[stack_size - 1 - i]));
            if (chars_written < 0)
                HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "cannot write char %c", c);
            *repr_len += 3;
        } /* end for each continuation byte */
    }     /* end else (multi-byte) */

    *(repr + *repr_len) = '\0';

done:
    return (ret_value);
} /* NCH5_s3comms_percent_encode_char */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_signing_key()
 * Purpose:
 *     Create AWS4 "Signing Key" from secret key, AWS region, and timestamp.
 *     Sequentially runs HMAC_SHA256 on strings in specified order,
 *     generating re-usable checksum (according to documentation, valid for
 *     7 days from time given).
 *     `secret` is `access key id` for targeted service/bucket/resource.
 *     `iso8601now` must conform to format, yyyyMMDD'T'hhmmss'Z'
 *     e.g. "19690720T201740Z".
 *     `region` should be one of AWS service region names, e.g. "us-east-1".
 *     Hard-coded "service" algorithm requirement to "s3".
 *     Inputs must be null-terminated strings.
 *     Writes to `md` the raw byte data, length of `SHA256_DIGEST_LENGTH`.
 *     Programmer must ensure that `md` is appropriately allocated.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - raw byte data of signing key written to `md`
 *     - FAILURE: `FAIL`
 *         - if any input arguments was NULL
 * Programmer: Jacob Smith
 *             2017-07-13
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_signing_key(unsigned char **mdp, const char *secret, const char *region, const char *iso8601now)
{
    char         *AWS4_secret     = NULL;
    size_t        AWS4_secret_len = 0;
    unsigned char datekey[SHA256_DIGEST_LENGTH];
    unsigned char dateregionkey[SHA256_DIGEST_LENGTH];
    unsigned char dateregionservicekey[SHA256_DIGEST_LENGTH];
    int           ret       = 0; /* return value of snprintf */
    int        ret_value = SUCCEED;
    unsigned char* md = NULL;

#if S3COMMS_DEBUG
    fprintf(stdout, "called NCH5_s3comms_signing_key.\n");
#endif

    if (mdp == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "Destination `mdp` cannot be NULL.");
    if (secret == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EAUTH, FAIL, "`secret` cannot be NULL.");
    if (region == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EAUTH, FAIL, "`region` cannot be NULL.");
    if (iso8601now == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "`iso8601now` cannot be NULL.");

    AWS4_secret_len = 4 + nulllen(secret) + 1;
    AWS4_secret     = (char *)malloc(sizeof(char *) * AWS4_secret_len);
    if (AWS4_secret == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "Could not allocate space.");

    /* prepend "AWS4" to start of the secret key */
    ret = snprintf(AWS4_secret, AWS4_secret_len, "%s%s", "AWS4", secret);
    if ((size_t)ret != (AWS4_secret_len - 1))
        HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL, "problem writing AWS4+secret `%s`", secret);

    if((md = (unsigned char*)calloc(1,SHA256_DIGEST_LENGTH))==NULL)
       HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, NULL, "could not malloc space for signing key .");

    /* hash_func, key, len(key), msg, len(msg), digest_dest, digest_len_dest
     * we know digest length, so ignore via NULL
     */
#if 0
    HMAC(EVP_sha256(), (const unsigned char *)AWS4_secret, (int)nulllen(AWS4_secret),
         (const unsigned char *)iso8601now, 8, /* 8 --> length of 8 --> "yyyyMMDD"  */
         datekey, NULL);
    HMAC(EVP_sha256(), (const unsigned char *)datekey, SHA256_DIGEST_LENGTH, (const unsigned char *)region,
         nulllen(region), dateregionkey, NULL);
    HMAC(EVP_sha256(), (const unsigned char *)dateregionkey, SHA256_DIGEST_LENGTH,
         (const unsigned char *)"s3", 2, dateregionservicekey, NULL);
    HMAC(EVP_sha256(), (const unsigned char *)dateregionservicekey, SHA256_DIGEST_LENGTH,
         (const unsigned char *)"aws4_request", 12, md, NULL);
#else
    Curl_hmacit(Curl_HMAC_SHA256, (const unsigned char *)AWS4_secret, (int)nulllen(AWS4_secret),
         (const unsigned char *)iso8601now, 8, /* 8 --> length of 8 --> "yyyyMMDD"  */
         datekey);
    Curl_hmacit(Curl_HMAC_SHA256, (const unsigned char *)datekey, SHA256_DIGEST_LENGTH, (const unsigned char *)region,
         nulllen(region), dateregionkey);
    Curl_hmacit(Curl_HMAC_SHA256, (const unsigned char *)dateregionkey, SHA256_DIGEST_LENGTH,
         (const unsigned char *)"s3", 2, dateregionservicekey);
    Curl_hmacit(Curl_HMAC_SHA256, (const unsigned char *)dateregionservicekey, SHA256_DIGEST_LENGTH,
         (const unsigned char *)"aws4_request", 12, md);
#endif
    if(mdp) {*mdp = md; md = NULL;}

done:
    nullfree(md);
    free(AWS4_secret);
    return (ret_value);
} /* end NCH5_s3comms_signing_key() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_tostringtosign()
 * Purpose:
 *     Get AWS "String to Sign" from Canonical Request, timestamp,
 *     and AWS "region".
 *     Common between single request and "chunked upload",
 *     conforms to:
 *         "AWS4-HMAC-SHA256\n" +
 *         <ISO8601 date format> + "\n" +  // yyyyMMDD'T'hhmmss'Z'
 *         <yyyyMMDD> + "/" + <AWS Region> + "/s3/aws4-request\n" +
 *         hex(SHA256(<CANONICAL-REQUEST>))
 *     Inputs `creq` (canonical request string), `now` (ISO8601 format),
 *     and `region` (s3 region designator string) must all be
 *     null-terminated strings.
 *     Result is written to `dest` with null-terminator.
 *     It is left to programmer to ensure `dest` has adequate space.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *         - "string to sign" written to `dest` and null-terminated
 *     - FAILURE: `FAIL`
 *         - if any of the inputs are NULL
 *         - if an error is encountered while computing checksum
 * Programmer: Jacob Smith
 *             2017-07-??
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_tostringtosign(VString* dest, const char *req, const char *now, const char *region)
{
    unsigned char checksum[SHA256_DIGEST_LENGTH * 2 + 1];
    char          day[9];
    char          hexsum[SHA256_DIGEST_LENGTH * 2 + 1];
    size_t        i         = 0;
    int           ret       = 0; /* snprintf return value */
    int           ret_value = SUCCEED;
    char          tmp[128];

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_tostringtosign.\n");
#endif

    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "destination buffer cannot be null.");
    if (req == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "canonical request cannot be null.");
    if (now == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "Timestring cannot be NULL.");
    if (region == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EAUTH, FAIL, "Region cannot be NULL.");

    for (i = 0; i < 128; i++)
        tmp[i] = '\0';
    for (i = 0; i < SHA256_DIGEST_LENGTH * 2 + 1; i++) {
        checksum[i] = '\0';
        hexsum[i]   = '\0';
    }
    strncpy(day, now, 8);
    day[8] = '\0';
    ret    = snprintf(tmp, 127, "%s/%s/s3/aws4_request", day, region);
    if (ret <= 0 || ret >= 127)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "problem adding day and region to string");

    vscat(dest,"AWS4-HMAC-SHA256\n");
    vsappendn(dest, now, nulllen(now));
    vscat(dest,"\n");

    vsappendn(dest, tmp, nulllen(tmp));
    vscat(dest,"\n");

#if 0
    SHA256((const unsigned char *)req, nulllen(req), checksum);
#else
    Curl_sha256it(checksum, (const unsigned char *)req, nulllen(req));
#endif

    if (NCH5_s3comms_bytes_to_hex(hexsum, (const unsigned char *)checksum, SHA256_DIGEST_LENGTH, TRUE) != SUCCEED)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not create hex string");

    vsappendn(dest,hexsum,SHA256_DIGEST_LENGTH * 2);

done:
    return (ret_value);
} /* end H5ros3_tostringtosign() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_trim()
 * Purpose:
 *     Remove all whitespace characters from start and end of a string `s`
 *     of length `s_len`, writing trimmed string copy to `dest`.
 *     Stores number of characters remaining at `n_written`.
 *     Destination for trimmed copy `dest` cannot be null.
 *     `dest` must have adequate space allocated for trimmed copy.
 *         If inadequate space, behavior is undefined, possibly resulting
 *         in segfault or overwrite of other data.
 *     If `s` is NULL or all whitespace, `dest` is untouched and `n_written`
 *     is set to 0.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 *         - `dest == NULL`
 * Programmer: Jacob Smith
 *             2017-09-18
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_trim(char *dest, char *s, size_t s_len, size_t *n_written)
{
    int ret_value = SUCCEED;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called NCH5_s3comms_trim.\n");
#endif

    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "destination cannot be null.");
    if (s == NULL)
        s_len = 0;

    if (s_len > 0) {
        /* Find first non-whitespace character from start;
         * reduce total length per character.
         */
        while ((s_len > 0) && isspace((unsigned char)s[0]) && s_len > 0) {
            s++;
            s_len--;
        }

        /* Find first non-whitespace character from tail;
         * reduce length per-character.
         * If length is 0 already, there is no non-whitespace character.
         */
        if (s_len > 0) {
            do {
                s_len--;
            } while (isspace((unsigned char)s[s_len]));
            s_len++;

            /* write output into dest */
            memcpy(dest, s, s_len);
        }
    }

    *n_written = s_len;

done:
    return (ret_value);
} /* end NCH5_s3comms_trim() */

/*----------------------------------------------------------------------------
 * Function: NCH5_s3comms_uriencode()
 * Purpose:
 *     URIencode (percent-encode) every byte except "[a-zA-Z0-9]-._~".
 *     For each character in source string `_s` from `s[0]` to `s[s_len-1]`,
 *     writes to `dest` either the raw character or its percent-encoded
 *     equivalent.
 *     See `NCH5_s3comms_bytes_to_hex` for information on percent-encoding.
 *     Space (' ') character encoded as "%20" (not "+")
 *     Forward-slash ('/') encoded as "%2F" only when `encode_slash == true`.
 *     Records number of characters written at `n_written`.
 *     Assumes that `dest` has been allocated with enough space.
 *     Neither `dest` nor `s` can be NULL.
 *     `s_len == 0` will have no effect.
 * Return:
 *     - SUCCESS: `SUCCEED`
 *     - FAILURE: `FAIL`
 *         - source strings `s` or destination `dest` are NULL
 *         - error while attempting to percent-encode a character
 * Programmer: Jacob Smith
 *             2017-07-??
 *----------------------------------------------------------------------------
 */
int
NCH5_s3comms_uriencode(char** destp, const char *s, size_t s_len, int encode_slash, size_t *n_written)
{
    char   c        = 0;
    char   hex_buffer[13];
    size_t hex_len   = 0;
    int ret_value = SUCCEED;
    size_t s_off     = 0;
    VString* dest = vsnew();    

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "NCH5_s3comms_uriencode called.\n");
#endif

    if (s == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "source string cannot be NULL");
    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "destination cannot be NULL");

    /* Write characters to destination, converting to percent-encoded
     * "hex-utf-8" strings if necessary.
     * e.g., '$' -> "%24"
     */
    for (s_off = 0; s_off < s_len; s_off++) {
        c = s[s_off];
        if (isalnum(c) || c == '.' || c == '-' || c == '_' || c == '~' ||
            (c == '/' && encode_slash == FALSE))
            vsappend(dest, c);
        else {
            if (NCH5_s3comms_percent_encode_char(hex_buffer, (const unsigned char)c, &hex_len) != SUCCEED) {
                hex_buffer[0] = c;
                hex_buffer[1] = 0;
                HGOTO_ERRORVA(H5E_ARGS, NC_EINVAL, FAIL,
                            "unable to percent-encode character \'%s\' "
                            "at %d in \"%s\"",
                            hex_buffer, (int)s_off, s);
            }
            vsappendn(dest, hex_buffer, hex_len);
        } /* end else (not a regular character) */
    }     /* end for each character */
    
    if(n_written) {*n_written = vslength(dest);}
    if(destp) {*destp = vsextract(dest);}

done:
    vsfree(dest);
    return (ret_value);
} /* NCH5_s3comms_uriencode */


/**************************************************/
/* Extensions */

static int
validate_url(NCURI* purl)
{
    int ret_value = SUCCEED;
    if (purl == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "parsed url cannot be null.");
    if (purl->host == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "parsed url must have non-null host.");
    if (purl->path == NULL)

        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "parsed url must have non-null resource.");
done:
    return (ret_value);
}

static int
validate_handle(s3r_t* handle, const char* url)
{
    int ret_value = SUCCEED;
    if (handle == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle cannot be null.");
    if (handle->magic != S3COMMS_S3R_MAGIC)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle has invalid magic.");
    if (handle->curlhandle == NULL)
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle has bad (null) curlhandle.");

done:
    return (ret_value);
}

static int
request_setup(s3r_t* handle, const char* url, HTTPVerb verb, struct s3r_cbstruct* sds)
{
    int ret_value = SUCCEED;
    CURL* curlh = handle->curlhandle;

    (void)trace(curlh,1);

    /* Common setup (possibly overridden below) */
    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_URL, url))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_URL).");

    switch (verb) {
    case HTTPGET:
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HTTPGET, 1L))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_HTTPGET).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_WRITEDATA, sds))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_WRITEDATA).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_WRITEFUNCTION, curlwritecallback))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_WRITEFUNCTION).");
        break;
    case HTTPPUT:
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_UPLOAD, 1L))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_UPLOAD).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_READDATA, sds))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_READDATA).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_READFUNCTION, curlreadcallback))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_READFUNCTION).");
        break;
    case HTTPHEAD:
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_NOBODY, 1L))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_NOBODY).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HEADERDATA, sds))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_HEADERDATA).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HEADERFUNCTION, curlheadercallback))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_HEADERFUNCTION).");
        break;
    case HTTPDELETE:
        if( CURLE_OK != curl_easy_setopt(curlh, CURLOPT_CUSTOMREQUEST, "DELETE"))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_CUSTOMREQUEST).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HEADERDATA, sds))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_HEADERDATA).");
        if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HEADERFUNCTION, curlheadercallback))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_HEADERFUNCTION).");
       break;
    case HTTPPOST:
    default: 
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "Illegal verb: %d.",(int)verb);
            break;
    }

done:
    return (ret_value);
}


/**
  otherheaders is a vector of (header,value) pairs
 */
static int
build_request(s3r_t* handle, NCURI* purl,
              const char* byterange,
              const char** otherheaders,
              VString* payload,
              HTTPVerb verb)
{
    int i,ret_value = SUCCEED;
    struct curl_slist *curlheaders   = NULL;
    hrb_node_t        *node          = NULL;
    hrb_t             *request       = NULL;
    struct tm         *now           = NULL;
    CURL              *curlh         = handle->curlhandle;
    VString           *authorization = vsnew();
    VString           *signed_headers = vsnew();
    VString*           creds = vsnew();
    VString           *canonical_request = vsnew();
    VString           *buffer1 = vsnew();
    VString           *buffer2 = vsnew();
    char               hexsum[SHA256_DIGEST_LENGTH * 2 + 1];
    char*              payloadsha256 = NULL; /* [SHA256_DIGEST_LENGTH * 2 + 1]; */
#if 0
    char buffer1[512 + 1]; /* -> Canonical Request -> Signature */
    char buffer2[256 + 1]; /* -> String To Sign -> Credential */
    char signed_headers[48 + 1]; /*
         * should be large enough for nominal listing:
         * "host;range;x-amz-content-sha256;x-amz-date"
         * + '\0', with "range;" possibly absent
         */
#endif
    char iso8601now[ISO8601_SIZE];

    /* zero start of strings */
#if 0
    authorization[0]  = 0;
    buffer1[0]        = 0;
    buffer2[0]        = 0;
    signed_headers[0] = 0;
#endif
    iso8601now[0]     = 0;

    
    /**** CREATE HTTP REQUEST STRUCTURE (hrb_t) ****/

    request = NCH5_s3comms_hrb_init_request((const char *)purl->path, "HTTP/1.1");
    if (request == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not allocate hrb_t request.");
    assert(request->magic == S3COMMS_HRB_MAGIC);

    /* These headers are independent of auth */
    if (SUCCEED != NCH5_s3comms_hrb_node_insert(request->headers, "Host", purl->host))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to set host header");

    if (byterange != NULL) {
            if (SUCCEED != NCH5_s3comms_hrb_node_insert(request->headers, "Range", byterange))
                HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to set range header");
    }

    /* Add other headers */
    if(otherheaders != NULL) {
            const char** hdrs = otherheaders;
            for(;*hdrs;hdrs+=2) {
                const char* key = (const char*)hdrs[0];
                const char* value = (const char*)hdrs[1];               
                if (SUCCEED != NCH5_s3comms_hrb_node_insert(request->headers, key, value))
                   HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to set host header");
            }
    }

    now = gmnow();
    if (ISO8601NOW(iso8601now, now) != (ISO8601_SIZE - 1))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not format ISO8601 time.");

    if (SUCCEED != NCH5_s3comms_hrb_node_insert(request->headers, "x-amz-date", (const char *)iso8601now))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to set x-amz-date header");

    /* Compute SHA256 of upload data, if any */
    if(verb == HTTPPUT && payload != NULL) {
            unsigned char sha256csum[SHA256_DIGEST_LENGTH];
#if 0
            SHA256((const unsigned char*)vscontents(payload),vslength(payload),sha256csum);
#else
            Curl_sha256it(sha256csum, (const unsigned char *)vscontents(payload), vslength(payload));
#endif
            if((SUCCEED != NCH5_s3comms_bytes_to_hex(hexsum,sha256csum,sizeof(sha256csum),1/*lowercase*/)))
                HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to compute hex form of payload.");
            payloadsha256 = hexsum;
    } else {/* Everything else has no body */
            payloadsha256 = EMPTY_SHA256;
    }
    if (SUCCEED != NCH5_s3comms_hrb_node_insert(request->headers, "x-amz-content-sha256", (const char *)payloadsha256))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to set x-amz-content-sha256 header");

    if (handle->signing_key != NULL) {
        /* authenticate request
         */
        /* char authorization[512 + 1];
         *   512 := approximate max length...
         *    67 <len("AWS4-HMAC-SHA256 Credential=///s3/aws4_request,"
         *           "SignedHeaders=,Signature=")>
         * +   8 <yyyyMMDD>
         * +  64 <hex(sha256())>
         * + 128 <max? len(secret_id)>
         * +  20 <max? len(region)>
         * + 128 <max? len(signed_headers)>
         */

        /**** VERIFY INFORMATION EXISTS ****/

        if (handle->region == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle must have non-null region.");
        if (handle->accessid == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle must have non-null accessid.");
        if (handle->accesskey == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle must have non-null accesskey.");
        if (handle->signing_key == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "handle must have non-null signing_key.");

        sortheaders(request->headers); /* ensure sorted order */

        /**** COMPUTE AUTHORIZATION ****/

        /* buffer1 -> canonical request */
        if (SUCCEED != NCH5_s3comms_aws_canonical_request(buffer1, signed_headers, verb, purl->query, payloadsha256, request))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "bad canonical request");
#if S3COMMS_DEBUG >= 2
        fprintf(stderr,"canonical_request=\n%s\n",vscontents(buffer1));
#endif
        /* buffer2->string-to-sign */
        if (SUCCEED != NCH5_s3comms_tostringtosign(buffer2, vscontents(buffer1), iso8601now, handle->region))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "bad string-to-sign");
#if S3COMMS_DEBUG >= 2
        fprintf(stderr,"stringtosign=\n%s\n",vscontents(buffer2));
#endif
        /* hexsum -> signature */
        if (SUCCEED != NCH5_s3comms_HMAC_SHA256(handle->signing_key, SHA256_DIGEST_LENGTH, vscontents(buffer2), vslength(buffer2), hexsum))
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "bad signature");
#if S3COMMS_DEBUG >= 2
        fprintf(stderr,"HMAX_SHA256=|%s|\n",hexsum);
#endif
        iso8601now[8] = 0; /* trim to yyyyMMDD */
        S3COMMS_FORMAT_CREDENTIAL(creds, handle->accessid, iso8601now, handle->region, "s3");
        if (vslength(creds) >= S3COMMS_MAX_CREDENTIAL_SIZE)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to format aws4 credential string");
#if S3COMMS_DEBUG >= 2
        fprintf(stderr,"Credentials=|%s|\n",vscontents(creds));
#endif

        vscat(authorization,"AWS4-HMAC-SHA256 Credential=");
        vsappendn(authorization,vscontents(creds),vslength(creds));
        vscat(authorization,",SignedHeaders=");
        vscat(authorization,vscontents(signed_headers));
        vscat(authorization,",Signature=");
        vsappendn(authorization,hexsum,2*SHA256_DIGEST_LENGTH);
#if S3COMMS_DEBUG >= 2
        fprintf(stderr,"Authorization=|%s|\n",vscontents(authorization));
#endif

        /* append authorization header to http request buffer */
        if (NCH5_s3comms_hrb_node_insert(request->headers, "authorization", (const char *)vscontents(authorization)) != SUCCEED)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to set Authorization header");

    } /* end if should authenticate (info provided) */

    sortheaders(request->headers); /* re-sort */

    /**** SET CURLHANDLE HTTP HEADERS FROM GENERATED DATA ****/

    for(i=0;i<vlistlength(request->headers);i++) {
        node = vlistget(request->headers,i);
        if(node != NULL) {
            assert(node->magic == S3COMMS_HRB_NODE_MAGIC);
            curlheaders = curl_slist_append(curlheaders, (const char *)node->cat);
            if (curlheaders == NULL)
                HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not append header to curl slist.");
            node = node->next;
	}
    }

    /* sanity-check */
    if (curlheaders == NULL)
            /* above loop was probably never run */
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "curlheaders was never populated.");

    /* Apparently, Chunked Transfer Encoding cannot be used, so disable it explicitly */
    if((curlheaders = curl_slist_append(curlheaders, "Transfer-Encoding:"))==NULL)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "could not disable Transfer-Encoding.");

    /* finally, set http headers in curl handle */
    if (curl_easy_setopt(curlh, CURLOPT_HTTPHEADER, curlheaders) != CURLE_OK)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL,
                        "error while setting CURL option (CURLOPT_HTTPHEADER).");

    /* We need to save the curlheaders so we can release them after the transfer
       (see https://curl.se/libcurl/c/CURLOPT_HTTPHEADER.html). */
    if(handle->curlheaders != NULL) {
        curl_slist_free_all(handle->curlheaders);
        handle->curlheaders = NULL;
    }
    assert(handle->curlheaders == NULL);
    handle->curlheaders = curlheaders;
    curlheaders = NULL;

done:
    vsfree(buffer1);
    vsfree(buffer2);
    vsfree(authorization);
    vsfree(signed_headers);
    vsfree(canonical_request);
    vsfree(creds);
    if (curlheaders != NULL) {
        curl_slist_free_all(curlheaders);
        curlheaders = NULL;
    }
    if (request != NULL) {
        if (SUCCEED != NCH5_s3comms_hrb_destroy(request))
            HDONE_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "cannot release header request structure");
	request = NULL;
    }
    return (ret_value);
}

static int
perform_request(s3r_t* handle, long* httpcodep)
{
    int ret_value = SUCCEED;
    CURL              *curlh         = NULL;
    CURLcode           p_status      = CURLE_OK;
    long httpcode = 0;
    char     curlerrbuf[CURL_ERROR_SIZE];

    curlerrbuf[0] = '\0';
    curlh = handle->curlhandle;

#if S3COMMS_CURL_VERBOSITY > 1
    /* CURL will print (to stdout) information for each operation */
    (void)trace(curlh,1);
#endif

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_ERRORBUFFER, curlerrbuf))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "problem setting error buffer");

    p_status = curl_easy_perform(curlh);

    /* Get response code */
    if (CURLE_OK != curl_easy_getinfo(curlh, CURLINFO_RESPONSE_CODE, &httpcode))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "problem getting response code");

#ifdef DEBUG
  {
char* eurl = NULL;
curl_easy_getinfo(curlh, CURLINFO_EFFECTIVE_URL, &eurl);
if(eurl == NULL) eurl = "";
printf(">>>> url=%s verb=%s httpcode=%d",eurl,handle->httpverb,(int)httpcode);
if(p_status != CURLE_OK)
printf(" errmsg=(%d) |%s|",(int)p_status,curl_easy_strerror(p_status));
printf("\n");
fflush(stdout);
  }
#endif

    if(p_status == CURLE_HTTP_RETURNED_ERROR) {
        /* signal ok , but return the bad error code */
        p_status = CURLE_OK;
    }

#if S3COMMS_CURL_VERBOSITY > 0
    /* In event of error, print detailed information to stderr
     * This is not the default behavior.
     */
    {
        if (p_status != CURLE_OK) {
            fprintf(stderr, "CURL ERROR CODE: %d\nHTTP CODE: %ld\n", p_status, httpcode);
            fprintf(stderr, "%s\n", curl_easy_strerror(p_status));
        }
    } /* verbose error reporting */
#endif
    if (p_status != CURLE_OK)
        HGOTO_ERROR(H5E_VFL, NC_EACCESS, FAIL, "curl cannot perform request");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_ERRORBUFFER, NULL)) /* reset */
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "problem unsetting error buffer");

    /* Should be safe to reclaim the curl headers */
    if(handle->curlheaders != NULL) {
        curl_slist_free_all(handle->curlheaders);
        handle->curlheaders = NULL;
    }
    assert(handle->curlheaders == NULL);

done:
    if(httpcodep) *httpcodep = httpcode;
    return (ret_value);
}

static int
curl_reset(s3r_t* handle)
{
    int ret_value = SUCCEED;
    CURL* curlh = handle->curlhandle;

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_NOBODY, NULL))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_NOBODY).");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HEADERDATA, NULL))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_HEADERDATA).");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HEADERFUNCTION, NULL))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_HEADERFUNCTION).");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_URL, NULL))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_URL).");

    if(CURLE_OK != curl_easy_setopt(curlh, CURLOPT_CUSTOMREQUEST, NULL))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "error while setting CURL option (CURLOPT_CUSTOMREQUEST).");

    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HTTPGET, 1L))
        HGOTO_ERROR(H5E_ARGS, NC_EINVAL, NULL, "error while setting CURL option (CURLOPT_HTTPGET).");

    /* clear any Range */
    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_RANGE, NULL))
        HDONE_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "cannot unset CURLOPT_RANGE");

    /* clear headers */
    if (CURLE_OK != curl_easy_setopt(curlh, CURLOPT_HTTPHEADER, NULL))
        HDONE_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "cannot unset CURLOPT_HTTPHEADER");

done:
    return (ret_value);
}

static int
build_range(size_t offset, size_t len, char** rangep)
{
    int ret_value = SUCCEED;
    char              *rangebytesstr = NULL;
    int                ret           = 0; /* working variable to check  */
                                          /* return value of snprintf  */
    if (len > 0) {
        rangebytesstr = (char *)malloc(sizeof(char) * (S3COMMS_MAX_RANGE_STRING_SIZE + 1));
        if (rangebytesstr == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, FAIL, "could not malloc range format string.");
        ret = snprintf(rangebytesstr, (S3COMMS_MAX_RANGE_STRING_SIZE), "bytes=%lld-%lld",
                         (long long)offset, (long long)(offset + len - 1));
        if (ret <= 0 || ret >= S3COMMS_MAX_RANGE_STRING_SIZE)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to format HTTP Range value");
    }
    else if (offset > 0) {
        rangebytesstr = (char *)malloc(sizeof(char) * (S3COMMS_MAX_RANGE_STRING_SIZE + 1));
        if (rangebytesstr == NULL)
            HGOTO_ERROR(H5E_ARGS, NC_ENOMEM, FAIL, "could not malloc range format string.");
        ret = snprintf(rangebytesstr, (S3COMMS_MAX_RANGE_STRING_SIZE), "bytes=%lld-", (long long)offset);
        if (ret <= 0 || ret >= S3COMMS_MAX_RANGE_STRING_SIZE)
            HGOTO_ERROR(H5E_ARGS, NC_EINVAL, FAIL, "unable to format HTTP Range value");
    }

    if(rangep) {*rangep = rangebytesstr; rangebytesstr = NULL;}

done:
    nullfree(rangebytesstr);
    return (ret_value);
}

static const char*
verbtext(HTTPVerb verb)
{
    switch(verb) {
    case HTTPGET: return "GET";
    case HTTPPUT: return "PUT";
    case HTTPPOST: return "POST";
    case HTTPHEAD: return "HEAD";
    case HTTPDELETE: return "DELETE";
    default: break;
    }
    return NULL;
}

/* Qsort comparison function */
static int
hdrcompare(const void* arg1, const void* arg2)
{
    hrb_node_t* n1 = *((hrb_node_t**)arg1);
    hrb_node_t* n2 = *((hrb_node_t**)arg2);
    return strcasecmp(n1->name,n2->name);
}

static int
sortheaders(VList* headers)
{
    int ret_value = SUCCEED;
    size_t nnodes;
    void** listcontents = NULL;

#if S3COMMS_DEBUG_TRACE
    fprintf(stdout, "called sortheaders.\n");
#endif

    if (headers == NULL || vlistlength(headers) < 2) return ret_value;
    nnodes = vlistlength(headers);
    listcontents = vlistcontents(headers);
    /* sort */
    qsort(listcontents, nnodes, sizeof(hrb_node_t*), hdrcompare);
    return (ret_value);
}

static int
httptonc(long httpcode)
{
    int stat = NC_NOERR;
    if(httpcode <= 99) stat = NC_EINTERNAL; /* should never happen */
    else if(httpcode <= 199)
        stat = NC_NOERR; /* I guess */
    else if(httpcode <= 299) {
        switch (httpcode) {
        default: stat = NC_NOERR; break;
        }
    } else if(httpcode <= 399)
        stat = NC_NOERR; /* ? */
    else if(httpcode <= 499) {
        switch (httpcode) {
        case 400: stat = NC_EINVAL; break;
        case 401: case 402: case 403:
            stat = NC_EAUTH; break;
        case 404: stat = NC_EEMPTY; break;
        default: stat = NC_EINVAL; break;
        }
    } else
        stat = NC_ES3;
    return stat;
}

/**************************************************/
/* Request Tracing */

static void
dump(const char *text, FILE *stream, unsigned char *ptr, size_t size)
{
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
#if 1
  fprintf(stream,"|%.*s|\n",(int)size,ptr);
#else
  {
  size_t i;
  size_t c;
  unsigned int width=0x10;
  
  for(i=0; i<size; i+= width) {
    fprintf(stream, "%4.4lx: ", (long)i);
#if 0
    /* show hex to the left */
    for(c = 0; c < width; c++) {
      if(i+c < size)
        fprintf(stream, "%02x ", ptr[i+c]);
      else
        fputs("   ", stream);
    }
#endif
    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
      char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
      fputc(x, stream);
    }
    fputc('\n', stream); /* newline */
  }
  }
#endif
}
 
static int
my_trace(CURL *handle, curl_infotype type, char *data, size_t size,void *userp)
{
  int dumpdata = 0;
  int ssl = 0;
  const char *text;
  (void)handle; /* prevent compiler warning */
  (void)userp;
 
  switch (type) {
  case CURLINFO_TEXT:
    dumpdata=1;
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;
  case CURLINFO_HEADER_OUT:
    dumpdata=1;
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    dumpdata=1;
    text = "=> Send data";
    break;
  case CURLINFO_HEADER_IN:
    dumpdata=1;
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    dumpdata=1;
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    if(!ssl) return 0;
    text = "=> Send SSL data";
    break;
  case CURLINFO_SSL_DATA_IN:
    if(!ssl) return 0;
    text = "<= Recv SSL data";
    break;
  }
  if(dumpdata)
      dump(text, stderr, (unsigned char *)data, size);
  return 0;
}

static int
trace(CURL* curl, int onoff)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;
    if(getenv("S3TRACE") == NULL) goto done;
    cstat = curl_easy_setopt(curl, CURLOPT_VERBOSE, onoff?1L:0L);
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    cstat = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
done:
    return (stat);
}

