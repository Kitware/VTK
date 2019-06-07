/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */

/**
 * Functions for inferring dataset model
 * @author Dennis Heimbigner
 */

#ifndef NCINFERMODEL_H
#define NCINFERMODEL_H

/* Define the io handler to be used to do lowest level
   access. This is above the libcurl level and below the
   dispatcher level. This is only used for remote
   datasets or for implementations where the implementation
   multiplexes more than one IOSP in a single dispatcher.
*/
#define NC_IOSP_FILE	(1)
#define NC_IOSP_MEMORY	(2)
#define NC_IOSP_DAP2	(3)
#define NC_IOSP_DAP4	(4)
#define NC_IOSP_UDF	(5) /*Placeholder since we do not know IOSP for UDF*/
#define NC_IOSP_HTTP	(6)

/* Track the information hat will help us
   infer how to access the data defined by
   path + omode.
*/
typedef struct NCmodel {
    int format; /* NC_FORMAT_XXX value */
    int impl; /* NC_FORMATX_XXX value */
    int iosp; /* NC_IOSP_XXX value (above) */
} NCmodel;

/* Keep compiler quiet */
struct NCURI;
struct NC_dispatch;

#if 0
/* return first IOSP or NULL if none */
EXTERNL int NC_urliosp(struct NCURI* u);
#endif

/* Infer model format and implementation */
EXTERNL int NC_infermodel(const char* path, int* omodep, int iscreate, int useparallel, void* params, NCmodel* model, char** newpathp);

/**
 * Provide a hidden interface to allow utilities
 * to check if a given path name is really a url.
 * If not, put null in basenamep, else put basename of the url
 * minus any extension into basenamep; caller frees.
 * Return 1 if it looks like a url, 0 otherwise.
 */
EXTERNL int nc__testurl(const char* path, char** basenamep);

#if 0
/* allow access url parse and params without exposing nc_url.h */
EXTERNL int NCDAP_urlparse(const char* s, void** dapurl);
EXTERNL void NCDAP_urlfree(void* dapurl);
EXTERNL const char* NCDAP_urllookup(void* dapurl, const char* param);

/* Ping a specific server */
EXTERNL int NCDAP2_ping(const char*);
EXTERNL int NCDAP4_ping(const char*);
#endif

#endif /*NCINFERMODEL_H*/
