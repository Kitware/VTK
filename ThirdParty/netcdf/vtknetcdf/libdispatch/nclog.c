/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#include "config.h"
#ifdef _MSC_VER
#include<io.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

extern FILE* fdopen(int fd, const char *mode);
 
#include "nclog.h"

#define PREFIXLEN 8
#define MAXTAGS 256
#define NCTAGDFALT "Log";


static int nclogginginitialized = 0;

static struct NCLOGGLOBAL {
    int nclogging;
    int ncsystemfile; /* 1 => we are logging to file we did not open */
    char* nclogfile;
    FILE* nclogstream;
} nclog_global = {0,0,NULL,NULL};

static const char* nctagset[] = {"Warning","Error","Note","Debug"};
static const int nctagsize = sizeof(nctagset)/sizeof(char*);

/* Forward */
static const char* nctagname(int tag);
 
/*!\defgroup NClog NClog Management
@{*/

/*!\internal
*/

void
ncloginit(void)
{
    const char* file;
    if(nclogginginitialized)
	return;
    nclogginginitialized = 1;
    memset(&nclog_global,0,sizeof(nclog_global));
    ncsetlogging(0);
    nclog_global.nclogfile = NULL;
    nclog_global.nclogstream = NULL;
    /* Use environment variables to preset nclogging state*/
    /* I hope this is portable*/
    file = getenv(NCENVFLAG);
    if(file != NULL && strlen(file) > 0) {
        if(nclogopen(file)) {
	    ncsetlogging(1);
	}
    }
}

/*!
Enable/Disable logging.

\param[in] tf If 1, then turn on logging, if 0, then turn off logging.

\return The previous value of the logging flag.
*/

int
ncsetlogging(int tf)
{
    int was;
    if(!nclogginginitialized) ncloginit();
    was = nclog_global.nclogging;
    nclog_global.nclogging = tf;
    return was;
}

/*!
Specify a file into which to place logging output.

\param[in] file The name of the file into which to place logging output.
If the file has the value NULL, then send logging output to
stderr.

\return zero if the open failed, one otherwise.
*/

int
nclogopen(const char* file)
{
    if(!nclogginginitialized) ncloginit();
    nclogclose();
    if(file == NULL || strlen(file) == 0) {
	/* use stderr*/
	nclog_global.nclogstream = stderr;
	nclog_global.nclogfile = NULL;
	nclog_global.ncsystemfile = 1;
    } else if(strcmp(file,"stdout") == 0) {
	/* use stdout*/
	nclog_global.nclogstream = stdout;
	nclog_global.nclogfile = NULL;
	nclog_global.ncsystemfile = 1;
    } else if(strcmp(file,"stderr") == 0) {
	/* use stderr*/
	nclog_global.nclogstream = stderr;
	nclog_global.nclogfile = NULL;
	nclog_global.ncsystemfile = 1;
    } else {
	int fd;
	nclog_global.nclogfile = strdup(file);
	nclog_global.nclogstream = NULL;
	/* We need to deal with this file carefully
	   to avoid unauthorized access*/
	fd = open(nclog_global.nclogfile,O_WRONLY|O_APPEND|O_CREAT,0600);
	if(fd >= 0) {
	    nclog_global.nclogstream = fdopen(fd,"a");
	} else {
	    free(nclog_global.nclogfile);
	    nclog_global.nclogfile = NULL;
	    nclog_global.nclogstream = NULL;
	    ncsetlogging(0);
	    return 0;
	}
	nclog_global.ncsystemfile = 0;
    }
    return 1;
}

void
nclogclose(void)
{
    if(!nclogginginitialized) ncloginit();
    if(nclog_global.nclogstream != NULL && !nclog_global.ncsystemfile) {
	fclose(nclog_global.nclogstream);
    }
    if(nclog_global.nclogfile != NULL) free(nclog_global.nclogfile);
    nclog_global.nclogstream = NULL;
    nclog_global.nclogfile = NULL;
    nclog_global.ncsystemfile = 0;
}

/*!
Send logging messages. This uses a variable
number of arguments and operates like the stdio
printf function.

\param[in] tag Indicate the kind of this log message.
\param[in] format Format specification as with printf.
*/

void
nclog(int tag, const char* fmt, ...)
{
    va_list args;
    const char* prefix;

    if(!nclogginginitialized) ncloginit();

    if(!nclog_global.nclogging || nclog_global.nclogstream == NULL) return;

    prefix = nctagname(tag);
    fprintf(nclog_global.nclogstream,"%s:",prefix);

    if(fmt != NULL) {
      va_start(args, fmt);
      vfprintf(nclog_global.nclogstream, fmt, args);
      va_end( args );
    }
    fprintf(nclog_global.nclogstream, "\n" );
    fflush(nclog_global.nclogstream);
}

void
ncvlog(int tag, const char* fmt, va_list ap)
{
    const char* prefix;

    if(!nclogginginitialized) ncloginit();

    if(!nclog_global.nclogging || nclog_global.nclogstream == NULL) return;

    prefix = nctagname(tag);
    fprintf(nclog_global.nclogstream,"%s:",prefix);

    if(fmt != NULL) {
      vfprintf(nclog_global.nclogstream, fmt, ap);
    }
    fprintf(nclog_global.nclogstream, "\n" );
    fflush(nclog_global.nclogstream);
}

void
nclogtext(int tag, const char* text)
{
    nclogtextn(tag,text,strlen(text));
}

/*!
Send arbitrarily long text as a logging message.
Each line will be sent using nclog with the specified tag.
\param[in] tag Indicate the kind of this log message.
\param[in] text Arbitrary text to send as a logging message.
*/

void
nclogtextn(int tag, const char* text, size_t count)
{
    NC_UNUSED(tag);
    if(!nclog_global.nclogging || nclog_global.nclogstream == NULL) return;
    fwrite(text,1,count,nclog_global.nclogstream);
    fflush(nclog_global.nclogstream);
}

static const char*
nctagname(int tag)
{
    if(tag < 0 || tag >= nctagsize)
	return "unknown";
    return nctagset[tag];
}

/**@}*/
