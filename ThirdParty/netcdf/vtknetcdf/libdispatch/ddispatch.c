/*
Copyright (c) 1998-2018 University Corporation for Atmospheric Research/Unidata
See LICENSE.txt for license information.
*/

#include "config.h"
#include "ncdispatch.h"
#include "ncuri.h"
#include "nclog.h"
#include "ncbytes.h"
#include "ncrc.h"
#include "ncoffsets.h"

/* Required for getcwd, other functions. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Required for getcwd, other functions. */
#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#endif

#if defined(ENABLE_BYTERANGE) || defined(ENABLE_DAP) || defined(ENABLE_DAP4)
#include <curl/curl.h>
#endif

/* Define vectors of zeros and ones for use with various nc_get_varX functions */
size_t NC_coord_zero[NC_MAX_VAR_DIMS] = {0};
size_t NC_coord_one[NC_MAX_VAR_DIMS] = {1};
ptrdiff_t NC_stride_one[NC_MAX_VAR_DIMS] = {1};

NCRCglobalstate ncrc_globalstate;

/*
static nc_type longtype = (sizeof(long) == sizeof(int)?NC_INT:NC_INT64);
static nc_type ulongtype = (sizeof(unsigned long) == sizeof(unsigned int)?NC_UINT:NC_UINT64);
*/

/* Allow dispatch to do general initialization and finalization */
int
NCDISPATCH_initialize(void)
{
    int status = NC_NOERR;
    int i;
    NCRCglobalstate* globalstate = NULL;

    for(i=0;i<NC_MAX_VAR_DIMS;i++) {
        NC_coord_zero[i] = 0;
        NC_coord_one[i]  = 1;
        NC_stride_one[i] = 1;
    }

    globalstate = ncrc_getglobalstate(); /* will allocate and clear */

    /* Capture temp dir*/
    {
	char* tempdir;
	char* p;
	char* q;
	char cwd[4096];
#ifdef _MSC_VER
        tempdir = getenv("TEMP");
#else
	tempdir = "/tmp";
#endif
        if(tempdir == NULL) {
	    fprintf(stderr,"Cannot find a temp dir; using ./\n");
	    tempdir = getcwd(cwd,sizeof(cwd));
	    if(tempdir == NULL || *tempdir == '\0') tempdir = ".";
	}
        globalstate->tempdir= (char*)malloc(strlen(tempdir) + 1);
	for(p=tempdir,q=globalstate->tempdir;*p;p++,q++) {
	    if((*p == '/' && *(p+1) == '/')
	       || (*p == '\\' && *(p+1) == '\\')) {p++;}
	    *q = *p;
	}
	*q = '\0';
#ifdef _MSC_VER
#else
        /* Canonicalize */
	for(p=globalstate->tempdir;*p;p++) {
	    if(*p == '\\') {*p = '/'; };
	}
	*q = '\0';
#endif
    }

    /* Capture $HOME */
    {
	char* p;
	char* q;
        char* home = getenv("HOME");

        if(home == NULL) {
	    /* use tempdir */
	    home = globalstate->tempdir;
	}
        globalstate->home = (char*)malloc(strlen(home) + 1);
	for(p=home,q=globalstate->home;*p;p++,q++) {
	    if((*p == '/' && *(p+1) == '/')
	       || (*p == '\\' && *(p+1) == '\\')) {p++;}
	    *q = *p;
	}
	*q = '\0';
#ifdef _MSC_VER
#else
        /* Canonicalize */
	for(p=home;*p;p++) {
	    if(*p == '\\') {*p = '/'; };
	}
#endif
    }

    /* Now load RC File */
    status = NC_rcload();
    ncloginit();

    /* Compute type alignments */
    NC_compute_alignments();

    /* Initialize curl if it is being used */
#if defined(ENABLE_BYTERANGE) || defined(ENABLE_DAP) || defined(ENABLE_DAP4)
    {
        CURLcode cstat = curl_global_init(CURL_GLOBAL_ALL);
	if(cstat != CURLE_OK)
	    status = NC_ECURL;
    }
#endif
    return status;
}

int
NCDISPATCH_finalize(void)
{
    int status = NC_NOERR;
    ncrc_freeglobalstate();
#if defined(ENABLE_BYTERANGE) || defined(ENABLE_DAP) || defined(ENABLE_DAP4)
    curl_global_cleanup();
#endif
    return status;
}
