/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef NCS3SDK_H
#define NCS3SDK_H 1

#ifdef __cplusplus
extern "C" {
#endif

EXTERNL int NC_s3sdkinitialize(void);
EXTERNL int NC_s3sdkfinalize(void);
EXTERNL void* NC_s3sdkcreateclient(NCS3INFO* context);
EXTERNL int NC_s3sdkbucketexists(void* s3client, const char* bucket, int* existsp, char** errmsgp);
EXTERNL int NC_s3sdkbucketcreate(void* s3client, const char* region, const char* bucket, char** errmsgp);
EXTERNL int NC_s3sdkbucketdelete(void* s3client, const char* region, const char* bucket, char** errmsgp);
EXTERNL int NC_s3sdkinfo(void* client0, const char* bucket, const char* pathkey, unsigned long long* lenp, char** errmsgp);
EXTERNL int NC_s3sdkread(void* client0, const char* bucket, const char* pathkey, unsigned long long start, unsigned long long count, void* content, char** errmsgp);
EXTERNL int NC_s3sdkwriteobject(void* client0, const char* bucket, const char* pathkey, unsigned long long count, const void* content, char** errmsgp);
EXTERNL int NC_s3sdkclose(void* s3client0, NCS3INFO* info, int deleteit, char** errmsgp);
EXTERNL int NC_s3sdkgetkeys(void* s3client0, const char* bucket, const char* prefix, size_t* nkeysp, char*** keysp, char** errmsgp);
EXTERNL int NC_s3sdksearch(void* s3client0, const char* bucket, const char* prefixkey0, size_t* nkeysp, char*** keysp, char** errmsgp);
EXTERNL int NC_s3sdkdeletekey(void* client0, const char* bucket, const char* pathkey, char** errmsgp);

#ifdef __cplusplus
}
#endif

#endif /*NCS3SDK_H*/
