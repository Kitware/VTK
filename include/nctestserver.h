/*! \file

Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014,
2015, 2016, 2017, 2018
University Corporation for Atmospheric Research/Unidata.

See \ref copyright file for more info.

*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "netcdf.h"

#undef FINDTESTSERVER_DEBUG

#define MAXSERVERURL 4096
#define TIMEOUT 10 /*seconds*/
#define BUFSIZE 8192 /*bytes*/
#define MAXREMOTETESTSERVERS 4096

#ifndef HAVE_CURLINFO_RESPONSE_CODE
#define CURLINFO_RESPONSE_CODE CURLINFO_HTTP_CODE
#endif

static int ping(const char* url);

static char**
parseServers(const char* remotetestservers)
{
    char* rts;
    char** servers = NULL;
    char** list = NULL;
    char* p;
    char* svc;
    char** l;
    size_t rtslen = strlen(remotetestservers);

    /* Keep LGTM quiet */
    if(rtslen > MAXREMOTETESTSERVERS) goto done;
    list = (char**)malloc(sizeof(char*) * (int)(rtslen/2));
    if(list == NULL) return NULL;
    rts = strdup(remotetestservers);
    if(rts == NULL) goto done;
    l = list;
    p = rts;
    for(;;) {
	svc = p;
	p = strchr(svc,',');
	if(p != NULL) *p = '\0';
	*l++ = strdup(svc);
	if(p == NULL) break;
	p++;
    }
    *l = NULL;
    servers = list;
    list = NULL;
done:
    if(rts) free(rts);
    if(list) free(list);
    return servers;
}

/**
Given a partial suffix path and a specified
protocol, test if a request to any of the test
servers + path returns some kind of result.
This indicates that the server is up and running.
Return the complete url for the server plus the path.
*/

static char*
nc_findtestserver(const char* path, int isdap4, const char* serverlist)
{
    char** svclist;
    char** svc;
    char url[MAXSERVERURL];
    char* match = NULL;
    int reportsearch;

    if((svclist = parseServers(serverlist))==NULL) {
	fprintf(stderr,"cannot parse test server list: %s\n",serverlist);
	return NULL;
    }
    reportsearch = (getenv("NC_REPORTSEARCH") != NULL);
    for(svc=svclist;*svc;svc++) {
	if(strlen(*svc) == 0)
	    goto done;
        if(path == NULL) path = "";
        if(strlen(path) > 0 && path[0] == '/')
	    path++;
	if(reportsearch)
	    fprintf(stderr,"nc_findtestserver: candidate=%s/%s: found=",*svc,path);
	/* Try https: first */
        snprintf(url,MAXSERVERURL,"https://%s/%s",*svc,path);
	if(ping(url) == NC_NOERR) {
	    if(reportsearch) fprintf(stderr,"yes\n");
	    match = strdup(url);
	    goto done;
	}
	/* Try http: next */
        snprintf(url,MAXSERVERURL,"http://%s/%s",*svc,path);
	if(ping(url) == NC_NOERR) {
	    if(reportsearch) fprintf(stderr,"yes\n");
	    match = strdup(url);
	    goto done;
	}
	if(reportsearch) fprintf(stderr,"no\n");
    }
done:
    if(reportsearch) fflush(stderr);
    /* Free up the envv list of servers */
    if(svclist != NULL) {
        char** p;
	for(p=svclist;*p;p++)
	    free(*p);
	free(svclist);
    }
    return match;
}

#define CERR(expr) if((cstat=(expr)) != CURLE_OK) goto done;

struct Buffer {
    char data[BUFSIZE];
    size_t offset; /* into buffer */
};

static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    struct Buffer* buffer = (struct Buffer*)data;
    size_t total = size * nmemb;
    size_t canwrite = total; /* assume so */
    if(total == 0) {
        fprintf(stderr,"WriteMemoryCallback: zero sized chunk\n");
	goto done;
    }
    if((buffer->offset + total) > sizeof(buffer->data))
	canwrite = (sizeof(buffer->data) - buffer->offset); /* partial read */
    if(canwrite > 0)
        memcpy(&(buffer->data[buffer->offset]),ptr,canwrite);
    buffer->offset += canwrite;
done:
    return total; /* pretend we captured everything */
}

/*
See if a server is responding.
Return NC_ECURL if the ping fails, NC_NOERR otherwise
*/
static int
ping(const char* url)
{
    int stat = NC_NOERR;
    CURLcode cstat = CURLE_OK;
    CURL* curl = NULL;
    long http_code = 0;
    struct Buffer data;

    /* Create a CURL instance */
    curl = curl_easy_init();
    if (curl == NULL) {cstat = CURLE_OUT_OF_MEMORY; goto done;}
    CERR((curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1)));

    /* Use redirects */
    CERR((curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L)));
    CERR((curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L)));

    /* use a very short timeout: 10 seconds */
    CERR((curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)TIMEOUT)));

    /* fail on HTTP 400 code errors */
    CERR((curl_easy_setopt(curl, CURLOPT_FAILONERROR, (long)1)));

    /* Set the URL */
    CERR((curl_easy_setopt(curl, CURLOPT_URL, (void*)url)));

    /* send all data to this function  */
    CERR((curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)));

    /* we pass our file to the callback function */
    CERR((curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data)));

    data.offset = 0;
    memset(data.data,0,sizeof(data.data));

    CERR((curl_easy_perform(curl)));

    /* Don't trust curl to return an error when request gets 404 */
    CERR((curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &http_code)));
    if(http_code >= 400) {
	cstat = CURLE_HTTP_RETURNED_ERROR;
	goto done;
    }

done:
    if(cstat != CURLE_OK) {
#ifdef FINDTESTSERVER_DEBUG
        fprintf(stderr, "curl error: %s; url=%s\n",
		curl_easy_strerror(cstat),url);
#endif
	stat = NC_ECURL;
    }
    if (curl != NULL)
        curl_easy_cleanup(curl);
    return stat;
}
