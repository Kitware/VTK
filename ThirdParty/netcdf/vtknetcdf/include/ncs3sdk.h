/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef NCS3SDK_H
#define NCS3SDK_H 1

#include "vtk_netcdf_mangle.h"

#define AWSHOST ".amazonaws.com"
#define GOOGLEHOST "storage.googleapis.com"

/* Define the "global" default region to be used if no other region is specified */
#define AWS_GLOBAL_DEFAULT_REGION "us-east-1"

/* Track the server type, if known */
typedef enum NCS3SVC {NCS3UNK=0, /* unknown */
                 NCS3=1,    /* s3.amazon.aws */
		 NCS3GS=2   /* storage.googleapis.com */
} NCS3SVC;

typedef struct NCS3INFO {
    char* host; /* non-null if other*/
    char* region; /* region */
    char* bucket; /* bucket name */
    char* rootkey;
    char* profile;
    NCS3SVC svc;
} NCS3INFO;

struct AWSentry {
    char* key;
    char* value;
};

struct AWSprofile {
    char* name;
    struct NClist* entries; /* NClist<struct AWSentry*> */
};

/* Opaque Types */
struct NClist;
struct NCglobalstate;

#ifdef __cplusplus
extern "C" {
#endif

/* API for ncs3sdk_XXX.[c|cpp] */
EXTERNL int NC_s3sdkinitialize(void);
EXTERNL int NC_s3sdkfinalize(void);
EXTERNL void* NC_s3sdkcreateclient(NCS3INFO* context);
EXTERNL int NC_s3sdkbucketexists(void* s3client, const char* bucket, int* existsp, char** errmsgp);
EXTERNL int NC_s3sdkbucketcreate(void* s3client, const char* region, const char* bucket, char** errmsgp);
EXTERNL int NC_s3sdkbucketdelete(void* s3client, NCS3INFO* info, char** errmsgp);
EXTERNL int NC_s3sdkinfo(void* client0, const char* bucket, const char* pathkey, unsigned long long* lenp, char** errmsgp);
EXTERNL int NC_s3sdkread(void* client0, const char* bucket, const char* pathkey, unsigned long long start, unsigned long long count, void* content, char** errmsgp);
EXTERNL int NC_s3sdkwriteobject(void* client0, const char* bucket, const char* pathkey, unsigned long long count, const void* content, char** errmsgp);
EXTERNL int NC_s3sdkclose(void* s3client0, NCS3INFO* info, int deleteit, char** errmsgp);
EXTERNL int NC_s3sdkgetkeys(void* s3client0, const char* bucket, const char* prefix, size_t* nkeysp, char*** keysp, char** errmsgp);
EXTERNL int NC_s3sdksearch(void* s3client0, const char* bucket, const char* prefixkey0, size_t* nkeysp, char*** keysp, char** errmsgp);
EXTERNL int NC_s3sdkdeletekey(void* client0, const char* bucket, const char* pathkey, char** errmsgp);

/* From ds3util.c */
EXTERNL void NC_s3sdkenvironment(void);

EXTERNL int NC_getdefaults3region(NCURI* uri, const char** regionp);
EXTERNL int NC_s3urlprocess(NCURI* url, NCS3INFO* s3, NCURI** newurlp);
EXTERNL int NC_s3clear(NCS3INFO* s3);
EXTERNL int NC_s3clone(NCS3INFO* s3, NCS3INFO** news3p);
EXTERNL const char* NC_s3dumps3info(NCS3INFO* info);
EXTERNL void NC_s3freeprofilelist(struct NClist* profiles);
EXTERNL int NC_getactives3profile(NCURI* uri, const char** profilep);
EXTERNL int NC_s3profilelookup(const char* profile, const char* key, const char** valuep);
EXTERNL int NC_authgets3profile(const char* profile, struct AWSprofile** profilep);
EXTERNL int NC_iss3(NCURI* uri, enum NCS3SVC*);
EXTERNL int NC_s3urlrebuild(NCURI* url, struct NCS3INFO* s3, NCURI** newurlp);
EXTERNL int NC_aws_load_credentials(struct NCglobalstate* gstate);

#ifdef __cplusplus
}
#endif

#endif /*NCS3SDK_H*/
