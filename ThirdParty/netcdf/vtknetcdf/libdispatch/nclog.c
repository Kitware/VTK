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
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
 
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include "netcdf.h"
#include "nclog.h"

#define PREFIXLEN 8
#define MAXTAGS 256
#define NCTAGDFALT "Log";

#define NC_MAX_FRAMES 256

static int nclogginginitialized = 0;

static struct NCLOGGLOBAL {
    int nclogging;
    int tracelevel;
    FILE* nclogstream;
    int depth;
    struct Frame {
	const char* fcn;
	int level;
	int depth;
    } frames[NC_MAX_FRAMES];
} nclog_global = {0,-1,NULL};

static const char* nctagset[] = {"Note","Warning","Error","Debug"};
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
    const char* envv = NULL;
    if(nclogginginitialized)
	return;
    nclogginginitialized = 1;
    memset(&nclog_global,0,sizeof(nclog_global));
    nclog_global.tracelevel = -1;    
    ncsetlogging(0);
    nclog_global.nclogstream = stderr;
    /* Use environment variables to preset nclogging state*/
    /* I hope this is portable*/
    envv = getenv(NCENVLOGGING);
    if(envv != NULL) {
	ncsetlogging(1);
    }
    envv = getenv(NCENVTRACING);
    if(envv != NULL) {
	nctracelevel(atoi(envv));
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
    if(nclog_global.nclogstream == NULL) nclogopen(NULL);
    return was;
}

int
nclogopen(FILE* stream)
{
    if(!nclogginginitialized) ncloginit();
    if(stream == NULL) stream = stderr;
    nclog_global.nclogstream = stream;
    return 1;
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
    if(fmt != NULL) {
      va_list args;
      va_start(args, fmt);
      ncvlog(tag,fmt,args);
      va_end(args);
    }
}

int
ncvlog(int tag, const char* fmt, va_list ap)
{
    const char* prefix;
    int was = -1;

    if(!nclogginginitialized) ncloginit();
    if(tag == NCLOGERR) was = ncsetlogging(1);
    if(!nclog_global.nclogging || nclog_global.nclogstream == NULL) return was;
    prefix = nctagname(tag);
    fprintf(nclog_global.nclogstream,"%s:",prefix);
    if(fmt != NULL) {
      vfprintf(nclog_global.nclogstream, fmt, ap);
    }
    fprintf(nclog_global.nclogstream, "\n" );
    fflush(nclog_global.nclogstream);
    return was;
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

/*!
Send trace messages.
\param[in] level Indicate the level of trace
\param[in] format Format specification as with printf.
*/

int
nctracelevel(int level)
{
    int oldlevel;
    if(!nclogginginitialized) ncloginit();
    oldlevel = nclog_global.tracelevel;
    if(level < 0) {
      nclog_global.tracelevel = level;
      ncsetlogging(0);
    } else { /*(level >= 0)*/
        nclog_global.tracelevel = level;
        ncsetlogging(1);
	nclogopen(NULL); /* use stderr */    
    }
    return oldlevel;
}

void
nctrace(int level, const char* fcn, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ncvtrace(level,fcn,fmt,args);
    va_end(args);
}

void
nctracemore(int level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ncvtrace(level,NULL,fmt,args);
    va_end(args);
}

void
ncvtrace(int level, const char* fcn, const char* fmt, va_list ap)
{
    struct Frame* frame;
    if(!nclogginginitialized) ncloginit();
    if(nclog_global.tracelevel < 0) ncsetlogging(0);
    if(fcn != NULL) {
        frame = &nclog_global.frames[nclog_global.depth];
        frame->fcn = fcn;
        frame->level = level;
        frame->depth = nclog_global.depth;
    }
    if(level <= nclog_global.tracelevel) {
	if(fcn != NULL)
            fprintf(nclog_global.nclogstream,"%s: (%d): %s:","Enter",level,fcn);
        if(fmt != NULL)
            vfprintf(nclog_global.nclogstream, fmt, ap);
        fprintf(nclog_global.nclogstream, "\n" );
        fflush(nclog_global.nclogstream);
    }
    if(fcn != NULL) nclog_global.depth++;
}

int
ncuntrace(const char* fcn, int err, const char* fmt, ...)
{
    va_list args;
    struct Frame* frame;
    va_start(args, fmt);
    if(nclog_global.depth == 0) {
	fprintf(nclog_global.nclogstream,"*** Unmatched untrace: %s: depth==0\n",fcn);
	goto done;
    }
    nclog_global.depth--;
    frame = &nclog_global.frames[nclog_global.depth];
    if(frame->depth != nclog_global.depth || strcmp(frame->fcn,fcn) != 0) {
	fprintf(nclog_global.nclogstream,"*** Unmatched untrace: fcn=%s expected=%s\n",frame->fcn,fcn);
	goto done;
    }
    if(frame->level <= nclog_global.tracelevel) {
        fprintf(nclog_global.nclogstream,"%s: (%d): %s: ","Exit",frame->level,frame->fcn);
	if(err)
	    fprintf(nclog_global.nclogstream,"err=(%d) '%s':",err,nc_strerror(err));
        if(fmt != NULL)
            vfprintf(nclog_global.nclogstream, fmt, args);
        fprintf(nclog_global.nclogstream, "\n" );
        fflush(nclog_global.nclogstream);
#ifdef HAVE_EXECINFO_H
        if(err != 0)
            ncbacktrace();
#endif
    }
done:
    va_end(args);
    if(err != 0)
        return ncbreakpoint(err);
    else
	return err;
}

int
ncbreakpoint(int err)
{
    return err;
}

#ifdef HAVE_EXECINFO_H
#define MAXSTACKDEPTH 100
void
ncbacktrace(void)
{
    int j, nptrs;
    void* buffer[MAXSTACKDEPTH];
    char **strings;

    if(getenv("NCBACKTRACE") == NULL) return;
    nptrs = backtrace(buffer, MAXSTACKDEPTH);
    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        errno = 0;
	return;
    }
    fprintf(stderr,"Backtrace:\n");
    for(j = 0; j < nptrs; j++)
	fprintf(stderr,"%s\n", strings[j]);
    free(strings);
}
#endif

/**@}*/
