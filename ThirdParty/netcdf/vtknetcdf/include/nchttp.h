/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */
/**
 * Header file for dhttp.c
 * @author Dennis Heimbigner
 */

#ifndef NCHTTP_H
#define NCHTTP_H

typedef enum HTTPMETHOD {
HTTPNONE=0, HTTPGET=1, HTTPPUT=2, HTTPPOST=3, HTTPHEAD=4
} HTTPMETHOD;

struct CURL; /* Forward */

typedef struct NC_HTTP_STATE {
    struct CURL* curl;
    long httpcode;        
    const char** headset; /* which headers to capture */
    NClist* headers;
    NCbytes* buf;
    char errbuf[1024]; /* assert(CURL_ERROR_SIZE <= 1024) */
} NC_HTTP_STATE;

extern int nc_http_open(const char* objecturl, NC_HTTP_STATE** state, long long* lenp);
extern int nc_http_size(NC_HTTP_STATE* state, const char* url, size64_t* sizep);
extern int nc_http_read(NC_HTTP_STATE* state, const char* url, size64_t start, size64_t count, NCbytes* buf);
extern int nc_http_close(NC_HTTP_STATE* state);
extern int nc_http_headers(NC_HTTP_STATE* state, const NClist** headersp); /* only if headerson */

#endif /*NCHTTP_H*/
