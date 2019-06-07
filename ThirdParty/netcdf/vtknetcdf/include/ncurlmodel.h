/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */

/**
 * Header file for dmode.c
 * @author Dennis Heimbigner
 */

#ifndef NCURLMODEL_H
#define NCURLMODEL_H

/* Define the io handler to be used to do lowest level
   access. This is above the libcurl level.
   Note that cases (DAP2,DAP4) where the implementation is 1-1
   with the iosp are not included.
*/
#define NC_IOSP_ZARR (1)

/* Track the information from a URL that will help us
   infer how to access the data pointed to by that URL.
*/
typedef struct NCmode {
    int format; /* NC_FORMAT_XXX value */
    int implementation; /* NC_FORMATX_XXX value */
    int iosp; /* NC_IOSP_XXX value (above) */
} NCmode;

/* return 1 if path looks like a url; 0 otherwise */
EXTERNL int NC_testurl(const char* path);

/*
Return an NC_FORMATX_... value.
Assumes that the path is known to be a url.
*/
EXTERNL int NC_urlmodel(const char* path, int mode, char** newurl, NCmode* model);

/**
 * Provide a hidden interface to allow utilities
 * to check if a given path name is really an ncdap3 url.
 * If no, put null in basenamep, else put basename of the url
 * minus any extension into basenamep; caller frees.
 * Return 1 if it looks like a url, 0 otherwise.
 */
EXTERNL int nc__testurl(const char* path, char** basenamep);

/* allow access url parse and params without exposing nc_url.h */
EXTERNL int NCDAP_urlparse(const char* s, void** dapurl);
EXTERNL void NCDAP_urlfree(void* dapurl);
EXTERNL const char* NCDAP_urllookup(void* dapurl, const char* param);

/* Ping a specific server */
EXTERNL int NCDAP2_ping(const char*);
EXTERNL int NCDAP4_ping(const char*);

#endif /*NCURLMODEL_H*/
