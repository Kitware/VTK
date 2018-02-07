#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "netcdf.h"

#define MAXSERVERURL 4096
#define TIMEOUT 10 /*seconds*/
#define BUFSIZE 8192 /*bytes*/

#ifndef HAVE_CURLINFO_RESPONSE_CODE
#define CURLINFO_RESPONSE_CODE CURLINFO_HTTP_CODE
#endif

static int ping(const char* url);

static char**
parseServers(const char* remotetestservers)
{
    char* rts;
    char** servers = NULL;
    char** list;
    char* p;
    char* svc;
    char** l;
    
    list = (char**)malloc(sizeof(char*) * (int)(strlen(remotetestservers)/2));
    if(list == NULL) return NULL;
    l = list;
    rts = strdup(remotetestservers);
    if(rts == NULL) {free(list); return NULL;}
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
    if(p) free(p);
    servers = list;
    return servers;
}

/**
Given a partial suffix path and a specified
protocol, test if a request to any of the test
servers + path returns some kind of result.
This indicates that the server is up and running.
Return the complete url for the server plus the path.
*/

char*
nc_findtestserver(const char* path, int isdap4, const char* serverlist)
{
    char** svc;
    char url[MAXSERVERURL];

    if((svc = parseServers(serverlist))==NULL) {
	fprintf(stderr,"cannot parse test server list: %s\n",serverlist);
	return NULL;
    }
    for(;*svc;svc++) {
	if(*svc == NULL || strlen(*svc) == 0)
	    return NULL;
        if(path == NULL) path = "";
        if(strlen(path) > 0 && path[0] == '/')
	    path++;
        snprintf(url,MAXSERVERURL,"http://%s/%s",*svc,path);
	if(ping(url) == NC_NOERR)
	    return strdup(url);
    }
    return NULL;
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
        fprintf(stderr, "curl error: %s", curl_easy_strerror(cstat));
	stat = NC_ECURL;
    }
    if (curl != NULL)
        curl_easy_cleanup(curl);
    return stat;
}
