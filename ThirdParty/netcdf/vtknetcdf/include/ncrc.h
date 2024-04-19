/*
Copyright (c) 1998-2018 University Corporation for Atmospheric Research/Unidata
See COPYRIGHT for license information.
*/

/*
Common functionality for reading
and accessing rc files (e.g. .daprc).
*/

#ifndef NCRC_H
#define NCRC_H

/* Need these support includes */
#include "ncuri.h"
#include "nclist.h"
#include "ncbytes.h"

/* getenv() keys */
#define NCRCENVIGNORE "NCRCENV_IGNORE"
#define NCRCENVRC "NCRCENV_RC"
#define NCRCENVHOME "NCRCENV_HOME"

/* Known .aws profile keys */
#define AWS_ACCESS_KEY_ID "aws_access_key_id"
#define AWS_SECRET_ACCESS_KEY "aws_secret_access_key"
#define AWS_REGION "aws_region"

typedef struct NCRCentry {
	char* host; /* combined host:port */
	char* urlpath; /* prefix to match or NULL */
        char* key;
        char* value;
} NCRCentry;

struct AWSentry {
    char* key;
    char* value;
};

struct AWSprofile {
    char* name;
    NClist* entries; /* NClist<struct AWSentry*> */
};

/* collect all the relevant info around the rc file and AWS */
typedef struct NCRCinfo {
	int ignore; /* if 1, then do not use any rc file */
	int loaded; /* 1 => already loaded */
        NClist* entries; /* the rc file entry store fields*/
        char* rcfile; /* specified rcfile; overrides anything else */
        char* rchome; /* Overrides $HOME when looking for .rc files */
	NClist* s3profiles; /* NClist<struct AWSprofile*> */
} NCRCinfo;

typedef struct NCS3INFO {
    char* host; /* non-null if other*/
    char* region; /* region */
    char* bucket; /* bucket name */
    char* rootkey;
    char* profile;
} NCS3INFO;

#if defined(__cplusplus)
extern "C" {
#endif

/* From drc.c */
EXTERNL void ncrc_initialize(void);
EXTERNL int NC_rcfile_insert(const char* key, const char* hostport, const char* path, const char* value);
EXTERNL char* NC_rclookup(const char* key, const char* hostport, const char* path);
EXTERNL char* NC_rclookupx(NCURI* uri, const char* key);

/* Following are primarily for debugging */
/* Obtain the count of number of entries */
EXTERNL size_t NC_rcfile_length(NCRCinfo*);
/* Obtain the ith entry; return NULL if out of range */
EXTERNL NCRCentry* NC_rcfile_ith(NCRCinfo*,size_t);

/* For internal use */
EXTERNL void NC_rcclear(NCRCinfo* info);
EXTERNL void NC_rcclear(NCRCinfo* info);

/* From dutil.c (Might later move to e.g. nc.h */
EXTERNL int NC__testurl(const char* path, char** basenamep);
EXTERNL int NC_isLittleEndian(void);
EXTERNL char* NC_entityescape(const char* s);
EXTERNL int NC_readfile(const char* filename, NCbytes* content);
EXTERNL int NC_readfilen(const char* filename, NCbytes* content, long long len);
EXTERNL int NC_readfileF(FILE* fp, NCbytes* content, long long len);
EXTERNL int NC_writefile(const char* filename, size_t size, void* content);
EXTERNL char* NC_mktmp(const char* base);
EXTERNL int NC_getmodelist(const char* modestr, NClist** modelistp);
EXTERNL int NC_testmode(NCURI* uri, const char* tag);
EXTERNL int NC_testpathmode(const char* path, const char* tag);
EXTERNL int NC_split_delim(const char* path, char delim, NClist* segments);
EXTERNL int NC_join(struct NClist* segments, char** pathp);

/* From ds3util.c */
/* S3 profiles */
EXTERNL int NC_s3urlrebuild(NCURI* url, NCURI** newurlp, char** bucketp, char** regionp);
EXTERNL int NC_getactives3profile(NCURI* uri, const char** profilep);
EXTERNL int NC_getdefaults3region(NCURI* uri, const char** regionp);
EXTERNL int NC_authgets3profile(const char* profile, struct AWSprofile** profilep);
EXTERNL int NC_s3profilelookup(const char* profile, const char* key, const char** valuep);
EXTERNL int NC_s3urlprocess(NCURI* url, NCS3INFO* s3);
EXTERNL int NC_s3clear(NCS3INFO* s3);
EXTERNL int NC_iss3(NCURI* uri);

#if defined(__cplusplus)
}
#endif

#endif /*NCRC_H*/
