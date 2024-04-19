/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */
/**
 * Header file for dhttp.c
 * @author Dennis Heimbigner
 */

#ifndef NCHTTP_H
#define NCHTTP_H

typedef enum HTTPMETHOD {
HTTPNONE=0, HTTPGET=1, HTTPPUT=2, HTTPPOST=3, HTTPHEAD=4, HTTPDELETE=5
} HTTPMETHOD;

struct CURL; /* Forward */

typedef struct NC_HTTP_STATE {
    struct CURL* curl;
    long httpcode;        
    struct Response {
        NClist* headset; /* which headers to capture */
        NClist* headers; /* Set of captured headers */
	NCbytes* buf; /* response content; call owns; do not free */
    } response;
    struct Request {
        HTTPMETHOD method;
	size_t payloadsize;
	void* payload; /* caller owns; do not free */
	size_t payloadpos;
        NClist* headers;
    } request;
    char errbuf[1024]; /* assert(CURL_ERROR_SIZE <= 1024) */
} NC_HTTP_STATE;

extern int nc_http_init(NC_HTTP_STATE** state);
extern int nc_http_init_verbose(NC_HTTP_STATE** state, int verbose);
extern int nc_http_size(NC_HTTP_STATE* state, const char* url, long long* sizep);
extern int nc_http_read(NC_HTTP_STATE* state, const char* url, size64_t start, size64_t count, NCbytes* buf);
extern int nc_http_write(NC_HTTP_STATE* state, const char* url, NCbytes* payload);
extern int nc_http_close(NC_HTTP_STATE* state);
extern int nc_http_reset(NC_HTTP_STATE* state);
extern int nc_http_set_method(NC_HTTP_STATE* state, HTTPMETHOD method);
extern int nc_http_set_response(NC_HTTP_STATE* state, NCbytes* buf);
extern int nc_http_set_payload(NC_HTTP_STATE* state, size_t len, void* payload);
extern int nc_http_response_headset(NC_HTTP_STATE* state, const NClist* headers); /* Set of headers to capture */
extern int nc_http_response_headers(NC_HTTP_STATE* state, NClist** headersp); /* set of captured headers */
extern int nc_http_request_setheaders(NC_HTTP_STATE* state, const NClist* headers); /* set of extra request headers */

#endif /*NCHTTP_H*/
