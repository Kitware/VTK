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
#include "ncuri.h"
#include "nchttp.h"
#include "ncauth.h"

#undef TRACE

#define CURLERR(e) reporterror(state,(e))

#if 0
static const char* LENGTH_ACCEPT[] = {"content-length","accept-ranges",NULL};
#endif
static const char* CONTENTLENGTH[] = {"content-length",NULL};

/* Forward */
static int setupconn(NC_HTTP_STATE* state, const char* objecturl);
static int execute(NC_HTTP_STATE* state);
static int headerson(NC_HTTP_STATE* state, const char** which);
static void headersoff(NC_HTTP_STATE* state);
static void showerrors(NC_HTTP_STATE* state);
static int reporterror(NC_HTTP_STATE* state, CURLcode cstat);
static int lookupheader(NC_HTTP_STATE* state, const char* key, const char** valuep);
static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size,void *userp);

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
@param curlp curl handle stored here if non-NULL
*/

int
nc_http_init(NC_HTTP_STATE** statep)
{
    return nc_http_init_verbose(statep,0);
}

int
nc_http_init_verbose(NC_HTTP_STATE** statep, int verbose)
{
    int stat = NC_NOERR;
    NC_HTTP_STATE* state = NULL;

    Trace("open");

    if((state = calloc(1,sizeof(NC_HTTP_STATE))) == NULL)
        {stat = NC_ENOMEM; goto done;}
    /* initialize curl*/
    state->curl = curl_easy_init();
    if (state->curl == NULL) {stat = NC_ECURL; goto done;}
    showerrors(state);
    if(verbose) {
        long onoff = 1;
        CURLcode cstat = CURLE_OK;
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_VERBOSE, onoff));
        if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
	cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_DEBUGFUNCTION, my_trace));
        if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    }
    stat = nc_http_reset(state);
    if(statep) {*statep = state; state = NULL;}

done:
    if(state) nc_http_close(state);
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
    nclistfreeall(state->response.headset); state->response.headset = NULL;
    nclistfreeall(state->response.headers); state->response.headers = NULL;
    ncbytesfree(state->response.buf);
    nclistfreeall(state->request.headers); state->request.headers = NULL;
    nullfree(state);
dbgflush();
    return stat;
}

/* Reset after a request */
int
nc_http_reset(NC_HTTP_STATE* state)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_HTTPGET, 1L));
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_NOBODY, 0L));
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_UPLOAD, 0L));
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    cstat = curl_easy_setopt(state->curl, CURLOPT_CUSTOMREQUEST, NULL);
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    cstat = curl_easy_setopt(state->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)-1);
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    state->request.method = HTTPGET;
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_WRITEFUNCTION, NULL));
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_WRITEDATA, NULL));
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_READFUNCTION, NULL));
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_READDATA, NULL));
    headersoff(state);
done:
    return stat;
}

/**************************************************/
/* Set misc parameters */

int
nc_http_set_method(NC_HTTP_STATE* state, HTTPMETHOD method)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;
    switch (method) {
    case HTTPGET:
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_HTTPGET, 1L));
	break;
    case HTTPHEAD:
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_HTTPGET, 1L));
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_NOBODY, 1L));
        break;
    case HTTPPUT:
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_UPLOAD, 1L));
	break;
    case HTTPDELETE:
	cstat = curl_easy_setopt(state->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_NOBODY, 1L));
	break;
    default: stat = NC_EINVAL; break;
    }
    if(cstat != CURLE_OK) {stat = NC_ECURL; goto done;}
    state->request.method = method;
done:
    return stat;
}

int
nc_http_set_payload(NC_HTTP_STATE* state, size_t size, void* payload)
{
    int stat = NC_NOERR;
    state->request.payloadsize = size;
    state->request.payload = payload;
    state->request.payloadpos = 0;
    return stat;
}

int
nc_http_set_response(NC_HTTP_STATE* state, NCbytes* buf)
{
    int stat = NC_NOERR;
    state->response.buf = buf;
    return stat;
}

/**************************************************/
/**
@param state state handle
@param objecturl to read
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

    if((stat = nc_http_set_response(state,buf))) goto fail;
    if((stat = setupconn(state,objecturl)))
	goto fail;

    /* Set to read byte range */
    snprintf(range,sizeof(range),"%ld-%ld",(long)start,(long)((start+count)-1));
    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_RANGE, range));
    if(cstat != CURLE_OK)
        {stat = NC_ECURL; goto done;}

    if((stat = execute(state)))
	goto done;
done:
    nc_http_reset(state);
    state->response.buf = NULL;
dbgflush();
    return stat;

fail:
    stat = NC_ECURL;
    goto done;
}

/**
@param state state handle
@param objecturl to write
@param payload send as body of a PUT
*/

int
nc_http_write(NC_HTTP_STATE* state, const char* objecturl, NCbytes* payload)
{
    int stat = NC_NOERR;

    Trace("write");

    if((stat = nc_http_set_payload(state,ncbyteslength(payload),ncbytescontents(payload)))) goto fail;
    if((stat = nc_http_set_method(state,HTTPPUT))) goto fail;
    if((stat = setupconn(state,objecturl))) goto fail;
    if((stat = execute(state)))
	goto done;
done:
    nc_http_reset(state);
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
nc_http_size(NC_HTTP_STATE* state, const char* objecturl, long long* sizep)
{
    int stat = NC_NOERR;
    const char* hdr = NULL;

    Trace("size");
    if(sizep == NULL)
	goto done; /* do not attempt to read */

    if((stat = nc_http_set_method(state,HTTPHEAD))) goto done;
    if((stat = setupconn(state,objecturl)))
	goto done;
    /* Make sure we get headers */
    if((stat = headerson(state,CONTENTLENGTH))) goto done;

    state->httpcode = 200;
    if((stat = execute(state)))
	goto done;

    if(nclistlength(state->response.headers) == 0)
	{stat = NC_EURL; goto done;}

    /* Get the content length header */
    if((stat = lookupheader(state,"content-length",&hdr))==NC_NOERR) {
	    sscanf(hdr,"%llu",sizep);
    }

done:
    nc_http_reset(state);
    headersoff(state);
dbgflush();
    return stat;
}

int
nc_http_response_headset(NC_HTTP_STATE* state, const NClist* keys)
{
    int i;
    if(keys == NULL) return NC_NOERR;
    if(state->response.headset == NULL)
        state->response.headset = nclistnew();
    for(i=0;i<nclistlength(keys);i++) {
	const char* key = (const char*)nclistget(keys,i);
	if(!nclistmatch(state->response.headset,key,0)) /* remove duplicates */
	    nclistpush(state->response.headset,strdup(key));
    }
    return NC_NOERR;
}

int
nc_http_response_headers(NC_HTTP_STATE* state, NClist** headersp)
{
    NClist* headers = NULL;
    if(headersp != NULL) {
        headers = nclistclone(state->response.headers,1);
        *headersp = headers; headers = NULL;
    }
    return NC_NOERR;
}

int
nc_http_request_setheaders(NC_HTTP_STATE* state, const NClist* headers)
{
    nclistfreeall(state->request.headers);
    state->request.headers = nclistclone(headers,1);
    return NC_NOERR;    
}

/**************************************************/

static size_t
ReadMemoryCallback(char* buffer, size_t size, size_t nmemb, void *data)
{
    NC_HTTP_STATE* state = data;
    size_t transfersize = size * nmemb;
    size_t avail = (state->request.payloadsize - state->request.payloadpos);

    Trace("ReadMemoryCallback");
    if(transfersize == 0)
        nclog(NCLOGWARN,"ReadMemoryCallback: zero sized buffer");
    if(transfersize > avail) transfersize = avail;
    memcpy(buffer,((char*)state->request.payload)+state->request.payloadpos,transfersize);
    state->request.payloadpos += transfersize;
    return transfersize;
}

static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    NC_HTTP_STATE* state = data;
    size_t realsize = size * nmemb;

    Trace("WriteMemoryCallback");
    if(realsize == 0)
        nclog(NCLOGWARN,"WriteMemoryCallback: zero sized chunk");
    ncbytesappendn(state->response.buf, ptr, realsize);
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
    const char* hdr;

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
    if(state->response.headset != NULL) {
        for(match=0,i=0;i<nclistlength(state->response.headset);i++) {
	    hdr = (const char*)nclistget(state->response.headset,i);
	    if(strcasecmp(hdr,name)==0) {match = 1; break;}        
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
    if(state->response.headers == NULL)
        state->response.headers = nclistnew();
    nclistpush(state->response.headers,name);
    name = NULL;
    if(value == NULL) value = strdup("");
    nclistpush(state->response.headers,value);
    value = NULL;
done:
    nullfree(name);
    return realsize;    
}

static int
setupconn(NC_HTTP_STATE* state, const char* objecturl)
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

    /* Pull some values from .rc tables */
    {
	NCURI* uri = NULL;
	char* hostport = NULL;
	char* value = NULL;
	ncuriparse(objecturl,&uri);
	if(uri == NULL) goto fail;
	hostport = NC_combinehostport(uri);
	ncurifree(uri); uri = NULL;
	value = NC_rclookup("HTTP.SSL.CAINFO",hostport,NULL);
	nullfree(hostport); hostport = NULL;	
	if(value == NULL)
	    value = NC_rclookup("HTTP.SSL.CAINFO",NULL,NULL);
	if(value != NULL) {
	    cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_CAINFO, value));
	    if (cstat != CURLE_OK) goto fail;
	}
    }

    /* Set the method */
    if((stat = nc_http_set_method(state,state->request.method))) goto done;

    if(state->response.buf) {
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
    if(state->request.payloadsize > 0) {
	state->request.payloadpos = 0; /* track reading */
	/* send all data to this function  */
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_READFUNCTION, ReadMemoryCallback));
        if (cstat != CURLE_OK) goto fail;
        /* Set argument for ReadMemoryCallback */
        cstat = CURLERR(curl_easy_setopt(state->curl, CURLOPT_READDATA, (void*)state));
        if (cstat != CURLE_OK) goto fail;
    } else {/* turn off data capture */
        (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_READFUNCTION, NULL));
        (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_READDATA, NULL));
    }

    /* Do method specific actions */
    switch(state->request.method) {
    case HTTPPUT:
	if(state->request.payloadsize > 0)
            cstat = curl_easy_setopt(state->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)state->request.payloadsize);
	break;
    default: break;
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
execute(NC_HTTP_STATE* state)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;

    cstat = CURLERR(curl_easy_perform(state->curl));
    if(cstat != CURLE_OK) goto fail;

    cstat = CURLERR(curl_easy_getinfo(state->curl,CURLINFO_RESPONSE_CODE,&state->httpcode));
    if(cstat != CURLE_OK) state->httpcode = 0;

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
    const char** p;

    if(state->response.headers != NULL)
	nclistfreeall(state->response.headers);
    state->response.headers = nclistnew();
    if(state->response.headset != NULL)
	nclistfreeall(state->response.headset);
    state->response.headset = nclistnew();
    for(p=headset;*p;p++)
	nclistpush(state->response.headset,strdup(*p));

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
    nclistfreeall(state->response.headers);
    state->response.headers = NULL;
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_HEADERFUNCTION, NULL));
    (void)CURLERR(curl_easy_setopt(state->curl, CURLOPT_HEADERDATA, NULL));
}

static int
lookupheader(NC_HTTP_STATE* state, const char* key, const char** valuep)
{
    int i;
    const char* value = NULL;
    /* Get the content length header */
    for(i=0;i<nclistlength(state->response.headers);i+=2) {
	char* s = nclistget(state->response.headers,i);
	if(strcasecmp(s,key)==0) {
	    value = nclistget(state->response.headers,i+1);
    	    break;
	}
    }
    if(value == NULL) return NC_ENOOBJECT;
    if(valuep)
	*valuep = value;
    return NC_NOERR;
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

static
void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size)
{
  size_t i;
  size_t c;
  unsigned int width=0x10;
 
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
 
  for(i=0; i<size; i+= width) {
    fprintf(stream, "%4.4lx: ", (long)i);
 
    /* show hex to the left */
    for(c = 0; c < width; c++) {
      if(i+c < size)
        fprintf(stream, "%02x ", ptr[i+c]);
      else
        fputs("   ", stream);
    }
 
    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
      char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
      fputc(x, stream);
    }
 
    fputc('\n', stream); /* newline */
  }
}
 
static int
my_trace(CURL *handle, curl_infotype type, char *data, size_t size,void *userp)
{
  const char *text;
  (void)handle; /* prevent compiler warning */
  (void)userp;
 
  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }
 
  dump(text, stderr, (unsigned char *)data, size);
  return 0;
}
