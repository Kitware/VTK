/**
 * @file
 *
 * Read a range of data from a remote dataset.
 *
 * Copyright 2018 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
*/

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define CURL_DISABLE_TYPECHECK 1
#include <curl/curl.h>

#include "netcdf.h"
#include "nclog.h"
#include "ncbytes.h"
#include "nclist.h"
#include "nchttp.h"

#undef VERBOSE
#undef TRACE

#define CURLERR(e) reporterror(state,(e))

/* Mnemonics */
#define GETCMD 0
#define HEADCMD 1

static const char* LENGTH_ACCEPT[] = {"content-length","accept-ranges",NULL};
static const char* CONTENTLENGTH[] = {"content-length",NULL};

/* Forward */
static int setupconn(NC_HTTP_STATE* state, const char* objecturl, NCbytes* buf);
static int execute(NC_HTTP_STATE* state, int headcmd);
static int headerson(NC_HTTP_STATE* state, const char** which);
static void headersoff(NC_HTTP_STATE* state);
static void showerrors(NC_HTTP_STATE* state);
static int reporterror(NC_HTTP_STATE* state, CURLcode cstat);

#ifdef TRACE
static void
dbgflush() {
fflush(stderr);
fflush(stdout);
}

static void
Trace(const char* fcn)
{
    fprintf(stdout,"xxx: %s\n",fcn);
    dbgflush();
}
#else
#define dbgflush()
#define Trace(fcn)
#endif /*TRACE*/

/**************************************************/

/**
@param objecturl url we propose to access
@param curlp curl handle stored here if non-NULL
@param filelenp store length of the file here if non-NULL
*/

int
nc_http_open(const char* objecturl, NC_HTTP_STATE** statep, long long* filelenp)
{
    int stat = NC_NOERR;
    int i;
    NC_HTTP_STATE* state = NULL;

    Trace("open");

    if((state = calloc(1,sizeof(NC_HTTP_STATE))) == NULL)
        {stat = NC_ENOMEM; goto done;}
    /* initialize curl*/
    state->curl = curl_easy_init();
    if (state->curl == NULL) {stat = NC_ECURL; goto done;}
    showerrors(state);
#ifdef VERBOSE
    {long onoff = 1;
    CURLcode cstat = CURLE_OK;
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_VERBOSE, onoff));
    if(cstat != CURLE_OK)
        {stat = NC_ECURL; goto done;}
    }
#endif
    if(filelenp) {
	*filelenp = -1;
        /* Attempt to get the file length using HEAD */
	if((stat = setupconn(state,objecturl,NULL))) goto done;
	if((stat = headerson(state,LENGTH_ACCEPT))) goto done;
	if((stat = execute(state,HEADCMD))) goto done;
	for(i=0;i<nclistlength(state->headers);i+=2) {
	    char* s = nclistget(state->headers,i);
	    if(strcasecmp(s,"content-length")==0) {
	        s = nclistget(state->headers,i+1);
		sscanf(s,"%lld",filelenp);
		break;
	    }
	    /* Also check for the Accept-Ranges header */ 
	    if(strcasecmp(s,"accept-ranges")==0) {
	        s = nclistget(state->headers,i+1);
		if(strcasecmp(s,"bytes")!=0) /* oops! */
		    {stat = NC_EURL; goto done;}
	    }
	}
	headersoff(state);
    }  
    if(statep) {*statep = state; state = NULL;}

done:
    nc_http_close(state);
dbgflush();
    return stat;
}

int
nc_http_close(NC_HTTP_STATE* state)
{
    int stat = NC_NOERR;

    Trace("close");

    if(state == NULL) return stat;
    if(state->curl != NULL)
	(void)curl_easy_cleanup(state->curl);
    nclistfreeall(state->headers); state->headers = NULL;
    if(state->buf !=  NULL)
        abort();
    nullfree(state);
dbgflush();
    return stat;
}

/**
Assume URL etc has already been set.
@param curl curl handle
@param start starting offset
@param count number of bytes to read
@param buf store read data here -- caller must allocate and free
*/

int
nc_http_read(NC_HTTP_STATE* state, const char* objecturl, size64_t start, size64_t count, NCbytes* buf)
{
    int stat = NC_NOERR;
    char range[64];
    CURLcode cstat = CURLE_OK;

    Trace("read");

    if(count == 0)
	goto done; /* do not attempt to read */

    if((stat = setupconn(state,objecturl,buf)))
	goto fail;

    /* Set to read byte range */
    snprintf(range,sizeof(range),"%ld-%ld",(long)start,(long)((start+count)-1));
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_RANGE, range));
    if(cstat != CURLE_OK)
        {stat = NC_ECURL; goto done;}

    if((stat = execute(state,GETCMD)))
	goto done;
done:
    state->buf = NULL;
dbgflush();
    return stat;

fail:
    stat = NC_ECURL;
    goto done;
}

/**
Return length of an object.
Assume URL etc has already been set.
@param curl curl handle
*/

int
nc_http_size(NC_HTTP_STATE* state, const char* objecturl, size64_t* sizep)
{
    int i,stat = NC_NOERR;

    Trace("size");
    if(sizep == NULL)
	goto done; /* do not attempt to read */

    if((stat = setupconn(state,objecturl,NULL)))
	goto done;
    /* Make sure we get headers */
    if((stat = headerson(state,CONTENTLENGTH))) goto done;

    state->httpcode = 200;
    if((stat = execute(state,HEADCMD)))
	goto done;

    if(nclistlength(state->headers) == 0)
	{stat = NC_EURL; goto done;}

    /* Get the content length header */
    for(i=0;i<nclistlength(state->headers);i+=2) {
	char* s = nclistget(state->headers,i);
	if(strcasecmp(s,"content-length")==0) {
	    s = nclistget(state->headers,i+1);
	    sscanf(s,"%llu",sizep);
    	    break;
	}
    }

done:
    headersoff(state);
dbgflush();
    return stat;
}

int
nc_http_headers(NC_HTTP_STATE* state, const NClist** headersp)
{
    if(headersp) *headersp = state->headers;
    return NC_NOERR;
}

/**************************************************/
static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    NC_HTTP_STATE* state = data;
    size_t realsize = size * nmemb;

    Trace("WriteMemoryCallback");
    if(realsize == 0)
        nclog(NCLOGWARN,"WriteMemoryCallback: zero sized chunk");
    ncbytesappendn(state->buf, ptr, realsize);
    return realsize;
}

static void
trim(char* s)
{
    size_t l = strlen(s);
    char* p = s;
    char* q = s + l;
    if(l == 0) return;
    q--; /* point to last char of string */
    /* Walk backward to first non-whitespace */
    for(;q > p;q--) {
	if(*q > ' ') break; /* found last non-whitespace */
    }
    /* invariant: p == q || *q > ' ' */
    if(p == q) /* string is all whitespace */
	{*p = '\0';}
    else {/* *q is last non-whitespace */
	q++; /* point to actual whitespace */
	*q = '\0';
    }
    /* Ok, skip past leading whitespace */
    for(p=s;*p;p++) {if(*p > ' ') break;}
    /* invariant: *p == '\0' || *p > ' ' */
    if(*p == 0) return; /* no leading whitespace */
    /* Ok, overwrite any leading whitespace */
    for(q=s;*p;) {*q++ = *p++;}
    *q = '\0';
    return;
}

static size_t
HeaderCallback(char *buffer, size_t size, size_t nitems, void *data)
{
    size_t realsize = size * nitems;
    char* name = NULL;
    char* value = NULL;
    char* p = NULL;
    size_t i;
    int havecolon;
    NC_HTTP_STATE* state = data;
    int match;
    const char** hdr;

    Trace("HeaderCallback");
    if(realsize == 0)
        nclog(NCLOGWARN,"HeaderCallback: zero sized chunk");
    i = 0;
    /* Look for colon separator */
    for(p=buffer;(i < realsize) && (*p != ':');p++,i++);
    havecolon = (i < realsize);
    if(i == 0)
        nclog(NCLOGWARN,"HeaderCallback: malformed header: %s",buffer);
    name = malloc(i+1);
    memcpy(name,buffer,i);
    name[i] = '\0';
    if(state->headset != NULL) {
        for(match=0,hdr=state->headset;*hdr;hdr++) {
	    if(strcasecmp(*hdr,name)==0) {match = 1; break;}        
        }
        if(!match) goto done;
    }
    /* Capture this header */
    value = NULL;
    if(havecolon) {
	size_t vlen = (realsize - i);
        value = malloc(vlen+1);
	p++; /* skip colon */
        memcpy(value,p,vlen);
        value[vlen] = '\0';
        trim(value);
    }
    if(state->headers == NULL)
        state->headers = nclistnew();
    nclistpush(state->headers,name);
    name = NULL;
    if(value == NULL) value = strdup("");
    nclistpush(state->headers,value);
    value = NULL;
done:
    nullfree(name);
    return realsize;    
}

static int
setupconn(NC_HTTP_STATE* state, const char* objecturl, NCbytes* buf)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;

    if(objecturl != NULL) {
        /* Set the URL */
#ifdef TRACE
        fprintf(stderr,"curl.setup: url |%s|\n",objecturl);
#endif
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_URL, (void*)objecturl));
        if (cstat != CURLE_OK) goto fail;
    }
    /* Set options */
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_TIMEOUT, 100)); /* 30sec timeout*/
    if (cstat != CURLE_OK) goto fail;
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_CONNECTTIMEOUT, 100));
    if (cstat != CURLE_OK) goto fail;
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_NOPROGRESS, 1));
    if (cstat != CURLE_OK) goto fail;
    cstat = curl_easy_setopt(state->curl, CURLOPT_FOLLOWLOCATION, 1); 
    if (cstat != CURLE_OK) goto fail;

    state->buf = buf;
    if(buf != NULL) {
	/* send all data to this function  */
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback));
        if (cstat != CURLE_OK) goto fail;
        /* Set argument for WriteMemoryCallback */
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_WRITEDATA, (void*)state));
        if (cstat != CURLE_OK) goto fail;
    } else {/* turn off data capture */
        (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_WRITEFUNCTION, NULL));
        (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_WRITEDATA, NULL));
    }

done:
    return stat;
fail:
    /* Turn off header capture */
    headersoff(state);
    stat = NC_ECURL;
    goto done;
}

static int
execute(NC_HTTP_STATE* state, int headcmd)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;

    if(headcmd) {
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_NOBODY, 1L));
        if(cstat != CURLE_OK) goto fail;
    }

    cstat = CURLERR(curl_easy_perform(state->curl));
    if(cstat != CURLE_OK) goto fail;

    cstat = CURLERR(curl_easy_getinfo(state->curl,CURLINFO_RESPONSE_CODE,&state->httpcode));
    if(cstat != CURLE_OK) state->httpcode = 0;

    if(headcmd) {
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_HTTPGET, 1L));
        if(cstat != CURLE_OK) goto fail;
    }

done:
    return stat;
fail:
    stat = NC_ECURL;
    goto done;
}

static int
headerson(NC_HTTP_STATE* state, const char** headset)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;

    if(state->headers != NULL)
	nclistfreeall(state->headers);
    state->headers = nclistnew();
    state->headset = headset;

    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_HEADERFUNCTION, HeaderCallback));
    if(cstat != CURLE_OK) goto fail;
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_HEADERDATA, (void*)state));
    if (cstat != CURLE_OK) goto fail;

done:
    return stat;
fail:
    stat = NC_ECURL;
    goto done;
}

static void
headersoff(NC_HTTP_STATE* state)
{
    nclistfreeall(state->headers);
    state->headers = NULL;
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_HEADERFUNCTION, NULL));
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_HEADERDATA, NULL));
}

static void
showerrors(NC_HTTP_STATE* state)
{
    (void)curl_easy_setopt(state->curl, CURLOPT_ERRORBUFFER, state->errbuf);
}
    
static int
reporterror(NC_HTTP_STATE* state, CURLcode cstat)
{
    if(cstat != CURLE_OK) 
        fprintf(stderr,"curlcode: (%d)%s : %s\n",
		cstat,curl_easy_strerror(cstat),state->errbuf);
    return cstat;
}
