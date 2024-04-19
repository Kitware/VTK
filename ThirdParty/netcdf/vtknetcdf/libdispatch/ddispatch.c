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
#include "ncpathmgr.h"
#include "ncxml.h"
#include "nc4internal.h"

/* Required for getcwd, other functions. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Required for getcwd, other functions. */
#ifdef _WIN32
#include <direct.h>
#endif

#if defined(ENABLE_BYTERANGE) || defined(ENABLE_DAP) || defined(ENABLE_DAP4)
#include <curl/curl.h>
#endif

#ifdef ENABLE_S3_SDK
#include "ncs3sdk.h"
#endif

#define MAXPATH 1024



/* Define vectors of zeros and ones for use with various nc_get_varX functions */
/* Note, this form of initialization fails under Cygwin */
size_t NC_coord_zero[NC_MAX_VAR_DIMS] = {0};
size_t NC_coord_one[NC_MAX_VAR_DIMS] = {1};
ptrdiff_t NC_stride_one[NC_MAX_VAR_DIMS] = {1};

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
    NCglobalstate* globalstate = NULL;

    for(i=0;i<NC_MAX_VAR_DIMS;i++) {
        NC_coord_zero[i] = 0;
        NC_coord_one[i]  = 1;
        NC_stride_one[i] = 1;
    }

    globalstate = NC_getglobalstate(); /* will allocate and clear */

    /* Capture temp dir*/
    {
	char* tempdir = NULL;
#if defined _WIN32 || defined __MSYS__ || defined __CYGWIN__
        tempdir = getenv("TEMP");
#else
	tempdir = "/tmp";
#endif
        if(tempdir == NULL) {
	    fprintf(stderr,"Cannot find a temp dir; using ./\n");
	    tempdir = ".";
	}
	globalstate->tempdir= strdup(tempdir);
    }

    /* Capture $HOME */
    {
#if defined(_WIN32) && !defined(__MINGW32__)
        char* home = getenv("USERPROFILE");
#else
        char* home = getenv("HOME");
#endif
        if(home == NULL) {
	    /* use cwd */
	    home = malloc(MAXPATH+1);
	    NCgetcwd(home,MAXPATH);
        } else
	    home = strdup(home); /* make it always free'able */
	assert(home != NULL);
        NCpathcanonical(home,&globalstate->home);
	nullfree(home);
    }
 
    /* Capture $CWD */
    {
        char cwdbuf[4096];

        cwdbuf[0] = '\0';
	(void)NCgetcwd(cwdbuf,sizeof(cwdbuf));

        if(strlen(cwdbuf) == 0) {
	    /* use tempdir */
	    strcpy(cwdbuf, globalstate->tempdir);
	}
        globalstate->cwd = strdup(cwdbuf);
    }

    ncloginit();

    /* Now load RC Files */
    ncrc_initialize();

    /* Compute type alignments */
    NC_compute_alignments();

#if defined(ENABLE_BYTERANGE) || defined(ENABLE_DAP) || defined(ENABLE_DAP4)
    /* Initialize curl if it is being used */
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
    NC_freeglobalstate();
#if defined(ENABLE_BYTERANGE) || defined(ENABLE_DAP) || defined(ENABLE_DAP4)
    curl_global_cleanup();
#endif
#if defined(ENABLE_DAP4)
   ncxml_finalize();
#endif
    return status;
}
