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

/* Required for getcwd, other functions. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Required for getcwd, other functions. */
#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#endif


/* Define vectors of zeros and ones for use with various nc_get_varX function*/
size_t nc_sizevector0[NC_MAX_VAR_DIMS];
size_t nc_sizevector1[NC_MAX_VAR_DIMS];
ptrdiff_t nc_ptrdiffvector1[NC_MAX_VAR_DIMS];
size_t NC_coord_zero[NC_MAX_VAR_DIMS];
size_t NC_coord_one[NC_MAX_VAR_DIMS];

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

    memset(&ncrc_globalstate,0,sizeof(NCRCglobalstate));

    for(i=0;i<NC_MAX_VAR_DIMS;i++) {
	nc_sizevector0[i] = 0;
        nc_sizevector1[i] = 1;
        nc_ptrdiffvector1[i] = 1;
    }
    for(i=0;i<NC_MAX_VAR_DIMS;i++) {
	NC_coord_one[i] = 1;
	NC_coord_zero[i] = 0;
    }

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
        ncrc_globalstate.tempdir= (char*)malloc(strlen(tempdir) + 1);
	for(p=tempdir,q=ncrc_globalstate.tempdir;*p;p++,q++) {
	    if((*p == '/' && *(p+1) == '/')
	       || (*p == '\\' && *(p+1) == '\\')) {p++;}
	    *q = *p;
	}
	*q = '\0';
#ifdef _MSC_VER
#else
        /* Canonicalize */
	for(p=ncrc_globalstate.tempdir;*p;p++) {
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
	    home = ncrc_globalstate.tempdir;
	}
        ncrc_globalstate.home = (char*)malloc(strlen(home) + 1);
	for(p=home,q=ncrc_globalstate.home;*p;p++,q++) {
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

    return status;
}

int
NCDISPATCH_finalize(void)
{
    int status = NC_NOERR;
    nullfree(ncrc_globalstate.tempdir);
    nullfree(ncrc_globalstate.home);
    NC_rcclear(&ncrc_globalstate.rcinfo);
    memset(&ncrc_globalstate,0,sizeof(NCRCglobalstate));
    return status;
}

