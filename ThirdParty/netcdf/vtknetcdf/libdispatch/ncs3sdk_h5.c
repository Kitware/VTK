/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#define NOOP
#undef DEBUG

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "netcdf.h"
#include "nclog.h"
#include "ncrc.h"
#include "ncxml.h"

#include "ncs3sdk.h"
#include "nch5s3comms.h"

#define NCTRACING
#ifdef NCTRACING
#define NCTRACE(level,fmt,...) nctrace((level),__func__,fmt,##__VA_ARGS__)
#define NCTRACEMORE(level,fmt,...) nctracemore((level),fmt,##__VA_ARGS__)
#define NCUNTRACE(e) ncuntrace(__func__,NCTHROW(e),NULL)
#define NCUNTRACEX(e,fmt,...) ncuntrace(__func__,NCTHROW(e),fmt,##__VA_ARGS__)
#define NCNILTRACE(e) ((void)NCUNTRACE(e))
#else
#define NCTRACE(level,fmt,...)
#define NCTRACEMORE(level,fmt,...)
#define NCUNTRACE(e) (e)
#define NCNILTRACE(e)
#define NCUNTRACEX(e,fmt,...) (e)
#endif

#ifdef __CYGWIN__
extern char* strdup(const char*);
#endif

#ifndef SHA256_DIGEST_LENGTH
#define SHA256_DIGEST_LENGTH 32
#endif

/* Mnemonic */
#define RECLAIM 1

#define size64 unsigned long long

typedef struct NCS3CLIENT {
    char*	rooturl;      /* The URL (minus any fragment) for the dataset root path (excludes bucket on down) */ 
    s3r_t*	h5s3client; /* From h5s3comms */  
} NCS3CLIENT;

struct Object {
    NClist* checksumalgorithms; /* NClist<char*> */
    char* etag;
    char* key;
    char* lastmodified;
    struct Owner {
        char* displayname;
        char* id;
    } owner;
    char* size;
    char* storageclass;
};

typedef char* CommonPrefix;

/* Hold essential info from a ListObjectsV2 response */
struct LISTOBJECTSV2 {
    char* istruncated;
    NClist* contents; /* NClist<struct Object*> */
    char* name;
    char* prefix;
    char* delimiter;
    char* maxkeys;
    NClist* commonprefixes; /* NClist<CommonPrefix> */
    char* encodingtype;
    char* keycount;
    char* continuationtoken;
    char* nextcontinuationtoken;
    char* startafter;
};

/* Forward */
static void s3client_destroy(NCS3CLIENT* s3client);
static char* makes3rooturl(NCS3INFO* info);
static int makes3fullpath(const char* pathkey, const char* bucket, const char* prefix, const char* key, NCbytes* url);
static int parse_listbucketresult(char* xml, unsigned long long xmllen, struct LISTOBJECTSV2**);
static int parse_object(ncxml_t root, NClist* objects);
static int parse_owner(ncxml_t root, struct Owner* ownerp);
static int parse_prefix(ncxml_t root, NClist* prefixes);
static int parse_checksumalgorithm(ncxml_t root, NClist* algorithms);
static struct LISTOBJECTSV2* alloclistobjectsv2(void);
static struct Object* allocobject(void);
static void reclaim_listobjectsv2(struct LISTOBJECTSV2* lo);
static void reclaim_object(struct Object* o);
static char* trim(char* s, int reclaim);
static int makes3prefix(const char* prefix, char** prefixdirp);
static int s3objectsinfo(NClist* contents, NClist* keys, NClist* lens);
static int s3commonprefixes(NClist* list, NClist* keys);
static int mergekeysets(NClist*,NClist*,NClist*);
static int rawtokeys(s3r_buf_t* response, NClist* keys, NClist* lengths, struct LISTOBJECTSV2** listv2p);

static int queryadd(NClist* query, const char* key, const char* value);
static int queryend(NClist* query, char** querystring);
static int queryinsert(NClist* list, char* ekey, char* evalue);

#define NT(x) ((x)==NULL?"null":x)

/**************************************************/

static int ncs3_initialized = 0;
static int ncs3_finalized = 0;

EXTERNL int
NC_s3sdkinitialize(void)
{
    if(!ncs3_initialized) {
	ncs3_initialized = 1;
	ncs3_finalized = 0;
    }

    /* Get environment information */
    NC_s3sdkenvironment();

    return NC_NOERR;
}

EXTERNL int
NC_s3sdkfinalize(void)
{
    if(!ncs3_finalized) {
	ncs3_initialized = 0;
	ncs3_finalized = 1;
    }
    return NC_NOERR;
}

/**************************************************/

#if 0
static void
dumps3info(NCS3INFO* s3info, const char* tag)
{
    if(tag == NULL) tag = "dumps3info";
    fprintf(stderr,">>> %s: s3info=%p\n",tag,s3info);
    if(s3info != NULL) {
        fprintf(stderr,">>> %s: s3info->host=%s\n",tag,NT(s3info->host));
        fprintf(stderr,">>> %s: s3info->region=%s\n",tag,NT(s3info->region));
        fprintf(stderr,">>> %s: s3info->bucket=%s\n",tag,NT(s3info->bucket));
        fprintf(stderr,">>> %s: s3info->rootkey=%s\n",tag,NT(s3info->rootkey));
        fprintf(stderr,">>> %s: s3info->profile=%s\n",tag,NT(s3info->profile));
    }
}

static void
dumps3client(void* s3client0, const char* tag)
{
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    if(tag == NULL) tag = "dumps3client";
    fprintf(stderr,">>> %s: s3client=%p\n",tag,s3client);
    if(s3client != NULL) {
	fprintf(stderr,">>> %s: s3client->rooturl=%s\n",tag,NT(s3client->rooturl));
	fprintf(stderr,">>> %s: s3client->h5s3client=%p\n",tag,s3client->rooturl);
    }
}
#endif

/**************************************************/

EXTERNL void*
NC_s3sdkcreateclient(NCS3INFO* info)
{
    int stat = NC_NOERR;
    const char* accessid = NULL;
    const char* accesskey = NULL;
    char* urlroot = NULL;
    NCS3CLIENT* s3client = NULL;

    NCTRACE(11,"info=%s",NC_s3dumps3info(info));

    s3client = (NCS3CLIENT*)calloc(1,sizeof(NCS3CLIENT));
    if(s3client == NULL) goto done;
    if(info->profile != NULL) {
        if((stat = NC_s3profilelookup(info->profile, "aws_access_key_id", &accessid))) goto done;
        if((stat = NC_s3profilelookup(info->profile, "aws_secret_access_key", &accesskey))) goto done;
    }
    if((s3client->rooturl = makes3rooturl(info))==NULL) {stat = NC_ENOMEM; goto done;}
    s3client->h5s3client = NCH5_s3comms_s3r_open(s3client->rooturl,info->svc,info->region,accessid,accesskey);
    if(s3client->h5s3client == NULL) {stat = NC_ES3; goto done;}

done:
    nullfree(urlroot);
    if(stat && s3client) {
        NC_s3sdkclose(s3client,info,0,NULL);
	s3client = NULL;
    }
    NCNILTRACE(NC_NOERR);
    return (void*)s3client;
}

EXTERNL int
NC_s3sdkbucketexists(void* s3client0, const char* bucket, int* existsp, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    NCbytes* url = ncbytesnew();
    long httpcode = 0;

    NCTRACE(11,"bucket=%s",bucket);
    if(errmsgp) *errmsgp = NULL;

    if((stat = makes3fullpath(s3client->rooturl,bucket,NULL,NULL,url))) goto done;
    if((stat = NCH5_s3comms_s3r_head(s3client->h5s3client, ncbytescontents(url), NULL, NULL, &httpcode, NULL))) goto done;

    if(existsp) {*existsp = (stat == 0 && httpcode == 200);}
done:
    ncbytesfree(url);
    return NCUNTRACEX(stat,"exists=%d",PTRVAL(int,existsp,-1));
}

EXTERNL int
NC_s3sdkbucketcreate(void* s3client0, const char* region, const char* bucket, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    
    NC_UNUSED(s3client);

    NCTRACE(11,"region=%s bucket=%s",region,bucket);
    if(errmsgp) *errmsgp = NULL;
    fprintf(stderr,"create bucket: %s\n",bucket); fflush(stderr);
    return NCUNTRACE(stat);    
}

EXTERNL int
NC_s3sdkbucketdelete(void* s3client0, NCS3INFO* info, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    
    NC_UNUSED(s3client);

    NCTRACE(11,"info=%s%s",NC_s3dumps3info(info));

    if(errmsgp) *errmsgp = NULL;
    fprintf(stderr,"delete bucket: %s\n",info->bucket); fflush(stderr);

    return NCUNTRACE(stat);    
}

/**************************************************/
/* Object API */

/*
@return NC_NOERR if key points to a content-bearing object.
@return NC_EEMPTY if object at key has no content.
@return NC_EXXX return true error
*/
EXTERNL int
NC_s3sdkinfo(void* s3client0, const char* bucket, const char* pathkey, size64_t* lenp, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    NCbytes* url = ncbytesnew();
    long long len = -1;

    NCTRACE(11,"bucket=%s pathkey=%s",bucket,pathkey);

    if((stat = makes3fullpath(s3client->rooturl,bucket,pathkey,NULL,url))) goto done;
    if((stat = NCH5_s3comms_s3r_getsize(s3client->h5s3client, ncbytescontents(url), &len))) goto done;

    if(lenp) {*lenp = len;}

done:
    ncbytesfree(url);
    return NCUNTRACEX(stat,"len=%d",PTRVAL(int,lenp,-1));
}

/*
@return NC_NOERR if success
@return NC_EXXX if fail
*/
EXTERNL int
NC_s3sdkread(void* s3client0, const char* bucket, const char* pathkey, size64_t start, size64_t count, void* content, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    NCbytes* url = ncbytesnew();
    struct s3r_buf_t data = {0,NULL};

    NCTRACE(11,"bucket=%s pathkey=%s start=%llu count=%llu content=%p",bucket,pathkey,start,count,content);

    if((stat = makes3fullpath(s3client->rooturl,bucket,pathkey,NULL,url))) goto done;

    /* Read the data */
    data.count = count;
    data.content = content;
    if((stat = NCH5_s3comms_s3r_read(s3client->h5s3client,ncbytescontents(url),(size_t)start,(size_t)count,&data))) goto done;
    
done:
    ncbytesfree(url);
    return NCUNTRACE(stat);
}

/*
For S3, I can see no way to do a byterange write;
so we are effectively writing the whole object
*/
EXTERNL int
NC_s3sdkwriteobject(void* s3client0, const char* bucket, const char* pathkey,  size64_t count, const void* content, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    NCbytes* url = ncbytesnew();
    s3r_buf_t data;

    NCTRACE(11,"bucket=%s pathkey=%s count=%llu content=%p",bucket,pathkey,count,content);

    if((stat = makes3fullpath(s3client->rooturl,bucket,pathkey,NULL,url))) goto done;

    /* Write the data */
    data.count = count;
    data.content = (void*)content;
    if((stat = NCH5_s3comms_s3r_write(s3client->h5s3client,ncbytescontents(url),&data))) goto done;
    
done:
    ncbytesfree(url);
    return NCUNTRACE(stat);
}

EXTERNL int
NC_s3sdkclose(void* s3client0, NCS3INFO* info, int deleteit, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;

    NCTRACE(11,"info=%s deleteit=%d",NC_s3dumps3info(info),deleteit);
    
    if(deleteit) {
        /* Delete the root key; ok it if does not exist */
        switch (stat = NC_s3sdkdeletekey(s3client0,info->bucket,info->rootkey,errmsgp)) {
        case NC_NOERR: break;
        case NC_EEMPTY: case NC_ENOTFOUND: stat = NC_NOERR; break;
        default: break;
        }
    }
    s3client_destroy(s3client);
    return NCUNTRACE(stat);
}

/*
Common code for getkeys and searchkeys.
Return a list of names of legal objects immediately below a specified key.
In theory, the returned list should be sorted in lexical order,
but it possible that it is not.
*/
static int
getkeys(void* s3client0, const char* bucket, const char* prefixkey0, const char* delim, size_t* nkeysp, char*** keysp, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    char* prefixdir = NULL;
    NClist* query = NULL;
    char* querystring = NULL;
    NCURI* purl = NULL;    
    NCbytes* listurl = ncbytesnew();
    NClist* allkeys = nclistnew();
    struct LISTOBJECTSV2* listv2 = NULL;
    int istruncated = 0;
    char* continuetoken = NULL;
    s3r_buf_t response = {0,NULL};

    NCTRACE(11,"bucket=%s prefixkey0=%s",bucket,prefixkey0);
    
    /* cleanup the prefix */
    if((stat = makes3prefix(prefixkey0,&prefixdir))) return NCUNTRACE(stat);        

    do {
	nclistfreeall(query);
        query = nclistnew();
	nullfree(querystring);
	querystring = NULL;
        ncbytesclear(listurl);
        nullfree(response.content); response.content = NULL; response.count = 0;
        /* Make sure order is sorted (after encoding) */
        if((stat = queryadd(query,"list-type","2"))) goto done;
        if((stat = queryadd(query,"prefix",prefixdir))) goto done;
	if(delim != NULL) {
            if((stat = queryadd(query,"delimiter",delim))) goto done;
        }
	if(istruncated && continuetoken != NULL) {
            if((stat = queryadd(query,"continuation-token",continuetoken))) goto done;
        }
        if((stat = queryend(query,&querystring))) goto done;

        /* Build the proper url; leave off prefix because it will appear in the query */
        if((stat = makes3fullpath(s3client->rooturl, bucket, NULL, NULL, listurl))) goto done;
        /* Append the query string */
        ncbytescat(listurl,"?");
        ncbytescat(listurl,querystring);

        if((stat = NCH5_s3comms_s3r_getkeys(s3client->h5s3client, ncbytescontents(listurl), &response))) goto done;
        if((stat = rawtokeys(&response,allkeys,NULL,&listv2))) goto done;
	istruncated = (strcasecmp(listv2->istruncated,"true")==0?1:0);
	nullfree(continuetoken);
	continuetoken = nulldup(listv2->nextcontinuationtoken);
        reclaim_listobjectsv2(listv2); listv2 = NULL;
    } while(istruncated);
    if(nkeysp) {*nkeysp = nclistlength(allkeys);}
    if(keysp) {*keysp = nclistextract(allkeys);}

done:
    nullfree(continuetoken);    
    reclaim_listobjectsv2(listv2);
    nclistfreeall(allkeys);
    nclistfreeall(query);
    nullfree(querystring);
    ncurifree(purl);
    ncbytesfree(listurl);
    nullfree(response.content);
    if(prefixdir) free(prefixdir);
    return NCUNTRACEX(stat,"nkeys=%u",PTRVAL(unsigned,nkeysp,0));
}

/*
Return a list of names of legal objects immediately below a specified key.
In theory, the returned list should be sorted in lexical order,
but it possible that it is not.
*/
EXTERNL int
NC_s3sdkgetkeys(void* s3client0, const char* bucket, const char* prefixkey0, size_t* nkeysp, char*** keysp, char** errmsgp)
{
    NCTRACE(11,"bucket=%s prefixkey0=%s",bucket,prefixkey0);
    return NCUNTRACE(getkeys(s3client0, bucket, prefixkey0, "/", nkeysp, keysp, errmsgp));
}

/*
Return a list of full keys  of legal objects immediately below a specified key.
Not necessarily sorted.
Essentially same as getkeys, but with no delimiter.
*/
EXTERNL int
NC_s3sdksearch(void* s3client0, const char* bucket, const char* prefixkey0, size_t* nkeysp, char*** keysp, char** errmsgp)
{
    NCTRACE(11,"bucket=%s prefixkey0=%s",bucket,prefixkey0);
    return NCUNTRACE(getkeys(s3client0, bucket, prefixkey0, NULL, nkeysp, keysp, errmsgp));
}

EXTERNL int
NC_s3sdkdeletekey(void* s3client0, const char* bucket, const char* pathkey, char** errmsgp)
{
    int stat = NC_NOERR;
    NCS3CLIENT* s3client = (NCS3CLIENT*)s3client0;
    NCbytes* url = ncbytesnew();
    long httpcode = 0;
    
    NCTRACE(11,"s3client0=%p bucket=%s pathkey=%s",s3client0,bucket,pathkey);

    if((stat = makes3fullpath(s3client->rooturl,bucket,pathkey,NULL,url))) goto done;

    if((stat = NCH5_s3comms_s3r_deletekey(s3client->h5s3client, ncbytescontents(url), &httpcode))) goto done;

done:
    ncbytesfree(url);
    return NCUNTRACE(stat);
}

/**************************************************/
/* Utilities */

/*
Convert raw getkeys response to vector of keys
*/
static int
rawtokeys(s3r_buf_t* response, NClist* allkeys, NClist* lengths, struct LISTOBJECTSV2** listv2p)
{
    int stat = NC_NOERR;
    struct LISTOBJECTSV2* listv2 = NULL;
    NClist* realkeys = nclistnew();
    NClist* commonkeys = nclistnew();

    if((stat = parse_listbucketresult(response->content,response->count,&listv2))) goto done;

    if(nclistlength(listv2->contents) > 0) {
        if((stat = s3objectsinfo(listv2->contents,realkeys,lengths))) goto done;
    }
    /* Add common prefixes */
    if(nclistlength(listv2->commonprefixes) > 0) {
        if((stat = s3commonprefixes(listv2->commonprefixes,commonkeys))) goto done;
    } 
    if((stat=mergekeysets(realkeys, commonkeys, allkeys))) goto done;

    if(listv2p) {*listv2p = listv2; listv2 = NULL;}
done:
    nclistfreeall(realkeys);
    nclistfreeall(commonkeys);
    reclaim_listobjectsv2(listv2);
    return stat;
}

/* Create a path-format root url */
static char*
makes3rooturl(NCS3INFO* info)
{
    NCbytes* buf = ncbytesnew();
    char* result = NULL;
    
    ncbytescat(buf,"https://");
    ncbytescat(buf,info->host);
    result = ncbytesextract(buf);
    ncbytesfree(buf);
    return result;
}

static int
makes3fullpath(const char* rooturl, const char* bucket, const char* prefix, const char* key, NCbytes* url)
{
    int stat = NC_NOERR;

    assert(url != NULL);
    assert(rooturl != NULL);

    ncbytescat(url,rooturl);

    if(bucket) {
        if(ncbyteslength(url) > 0 && ncbytesget(url,ncbyteslength(url)-1) != '/') ncbytescat(url,"/");
        if(*bucket == '/') bucket++;
        ncbytescat(url,bucket);
    }
    if(prefix) {
        if(ncbyteslength(url) > 0 && ncbytesget(url,ncbyteslength(url)-1) != '/') ncbytescat(url,"/");
        if(*prefix == '/') prefix++;
        ncbytescat(url,prefix);
    }
    if(key) {
        if(ncbyteslength(url) > 0 && ncbytesget(url,ncbyteslength(url)-1) != '/') ncbytescat(url,"/");
        if(*key == '/') key++;
        ncbytescat(url,key);
    }
    /* Remove any trailing '/' */
    if(ncbyteslength(url) > 0 && ncbytesget(url,ncbyteslength(url)-1) == '/') ncbytessetlength(url,ncbyteslength(url)-1);
    return stat;
}

/* (1) Ensure trailing '/' and (2) Strip off leading '/' */
/* Note that order of '/' handling is important; we want
   "/" to be converted to "", but "/a/b/c" to be converted to "a/b/c/"
*/
static int
makes3prefix(const char* prefix, char** prefixdirp)
{
    size_t plen;
    char* prefixdir;

    if(prefix == NULL) prefix = "";
    if(*prefix == '/') prefix++; /* Remove any leading '/' */
    plen = strlen(prefix);
    prefixdir = (char*)malloc(plen+1+1); /* possible '/' and nul */
    if(prefixdir == NULL) return NC_ENOMEM;
    memcpy(prefixdir,prefix,plen+1); /* include trailing nul */
    if(plen > 0)
	if(prefixdir[plen-1] != '/') strlcat(prefixdir,"/",plen+1+1); /* Ensure trailing '/' */
    /* else leave prefix as "" */
    *prefixdirp = prefixdir; prefixdir = NULL;
    return NC_NOERR;
}

/* Move keys1 concat keys2 into merge; note that merge list may not be empty. */
/* Will leave keys1 and keys2 empty */
static int
mergekeysets(NClist* keys1, NClist* keys2, NClist* merge)
{
    int stat = NC_NOERR;
    int i;
    size_t nkeys1 = nclistlength(keys1);
    size_t nkeys2 = nclistlength(keys2);
    for(i=0;i<nkeys1;i++) nclistpush(merge,nclistremove(keys1,0));		
    for(i=0;i<nkeys2;i++) nclistpush(merge,nclistremove(keys2,0));
    nclistnull(merge);
    return NCTHROW(stat);
}

static void
s3client_destroy(NCS3CLIENT* s3client)
{
    if(s3client) {
	nullfree(s3client->rooturl);
        (void)NCH5_s3comms_s3r_close(s3client->h5s3client);
        free(s3client);
    }
}

/**************************************************/
/* XML Response Parser(s) */

/**
Action: List objects (V2)
Response XML:
=========================
HTTP/1.1 200
<?xml version="1.0" encoding="UTF-8"?>
<ListBucketResult>
   <Name>string</Name>
   <Prefix>string</Prefix>
   <KeyCount>integer</KeyCount>
   <MaxKeys>integer</MaxKeys>
   <Delimiter>string</Delimiter>
   <IsTruncated>boolean</IsTruncated>
   <Contents>
      <Key>string</Key>
      <LastModified>timestamp</LastModified>
      <ETag>string</ETag>
      <Size>integer</Size>
      <StorageClass>string</StorageClass>
      <ChecksumAlgorithm>string</ChecksumAlgorithm>
      <Owner>
         <DisplayName>string</DisplayName>
         <ID>string</ID>
      </Owner>
#ifdef GOOGLES3
      <Generation>string</Generation>
      <MetaGeneration>string</MetaGeneration>
#endif
      ...
   </Contents>
   ...
   <CommonPrefixes>
      <Prefix>string</Prefix>
   </CommonPrefixes>
   ...
   <EncodingType>string</EncodingType>
   <ContinuationToken>string</ContinuationToken>
   <NextContinuationToken>string</NextContinuationToken>
   <StartAfter>string</StartAfter>
</ListBucketResult>
=========================
*/

static int
parse_listbucketresult(char* xml, unsigned long long xmllen, struct LISTOBJECTSV2** resultp)
{
    int stat = NC_NOERR;
    ncxml_doc_t doc = NULL;
    ncxml_t x;
    struct LISTOBJECTSV2* result = NULL;

#ifdef DEBUG
    printf("-------------------------%s\n-------------------------\n",xml);
#endif

    doc = ncxml_parse(xml,xmllen);
    if(doc == NULL) {stat = NC_ES3; goto done;}
    ncxml_t dom = ncxml_root(doc);

    /* Verify top level element */
    if(strcmp(ncxml_name(dom),"ListBucketResult")!=0) {
	nclog(NCLOGERR,"Expected: <ListBucketResult> actual: <%s>",ncxml_name(dom));
	stat = NC_ES3;
	goto done;
    }
    if((result = alloclistobjectsv2())==NULL) {stat = NC_ENOMEM; goto done;}

    /* Iterate next-level elements */
    for(x=ncxml_child_first(dom);x != NULL;x=ncxml_child_next(x)) {
	const char* elem = ncxml_name(x);
	if(strcmp(elem,"IsTruncated")==0) {
	    result->istruncated = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"Contents")==0) {
	    if((stat = parse_object(x,result->contents))) goto done;
	} else if(strcmp(elem,"Name")==0) {
	    result->name = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"Prefix")==0) {
	    result->prefix = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"Delimiter")==0) {
	    result->delimiter = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"MaxKeys")==0) {
	    result->maxkeys = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"CommonPrefixes")==0) {
	    if((stat = parse_prefix(x,result->commonprefixes))) goto done;
	} else if(strcmp(elem,"EncodingType")==0) {
	    result->encodingtype = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"KeyCount")==0) {
	    result->keycount = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"ContinuationToken")==0) {
	    result->continuationtoken = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"NextContinuationToken")==0) {
	    result->nextcontinuationtoken = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"StartAfter")==0) {
	    result->startafter = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"StartAfter")==0) {
	    result->startafter = trim(ncxml_text(x),RECLAIM);
	} else {
	    nclog(NCLOGERR,"Unexpected Element: <%s>",elem);
	    stat = NC_ES3;
	    goto done;
	}
    }
    if(resultp) {*resultp = result; result = NULL;}

done:
    if(result) reclaim_listobjectsv2(result);
    if(doc) ncxml_free(doc);
    return NCTHROW(stat);
}

static int
parse_object(ncxml_t root, NClist* objects)
{
    int stat = NC_NOERR;
    ncxml_t x;
    struct Object* object = NULL;

    /* Verify top level element */
    if(strcmp(ncxml_name(root),"Contents")!=0) {
	nclog(NCLOGERR,"Expected: <Contents> actual: <%s>",ncxml_name(root));
	stat = NC_ES3;
	goto done;
    }

    if((object = allocobject())==NULL) {stat = NC_ENOMEM; goto done;}

    for(x=ncxml_child_first(root);x != NULL;x=ncxml_child_next(x)) {
	const char* elem = ncxml_name(x);
	if(strcmp(elem,"ChecksumAlgorithm")==0) {
	    if((stat = parse_checksumalgorithm(x,object->checksumalgorithms))) goto done;
	} else if(strcmp(elem,"ETag")==0) {
	    object->etag = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"Key")==0) {
	    object->key = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"LastModified")==0) {
	    object->lastmodified = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"Owner")==0) {
	    if((stat = parse_owner(x,&object->owner))) goto done;
	} else if(strcmp(elem,"Size")==0) {
	    object->size = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"StorageClass")==0) {
	    object->storageclass = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"Generation")==0) {
	    /* Ignore */
	} else if(strcmp(elem,"MetaGeneration")==0) {
	    /* Ignore */
	} else {
	    nclog(NCLOGERR,"Unexpected Element: <%s>",elem);
	    stat = NC_ES3;
	    goto done;
	}
    }
    nclistpush(objects,object);
    object = NULL;

done:
    if(object) reclaim_object(object);
    return NCTHROW(stat);
}

static int
parse_owner(ncxml_t root, struct Owner* owner)
{
    int stat = NC_NOERR;
    ncxml_t x;

    /* Verify top level element */
    if(strcmp(ncxml_name(root),"Owner")!=0) {
	nclog(NCLOGERR,"Expected: <Owner> actual: <%s>",ncxml_name(root));
	stat = NC_ES3;
	goto done;
    }

    for(x=ncxml_child_first(root);x != NULL;x=ncxml_child_next(x)) {
	const char* elem = ncxml_name(x);
	if(strcmp(elem,"DisplayName")==0) {
	    owner->displayname = trim(ncxml_text(x),RECLAIM);
	} else if(strcmp(elem,"ID")==0) {
	    owner->id = trim(ncxml_text(x),RECLAIM);
	} else {
	    nclog(NCLOGERR,"Unexpected Element: <%s>",elem);
	    stat = NC_ES3;
	    goto done;
	}
    }

done:
    return NCTHROW(stat);
}

static int
parse_prefix(ncxml_t root, NClist* prefixes)
{
    int stat = NC_NOERR;
    ncxml_t x;
    char* prefix = NULL;

    /* Verify top level element */
    if(strcmp(ncxml_name(root),"CommonPrefixes")!=0) {
	nclog(NCLOGERR,"Expected: <CommonPrefixes> actual: <%s>",ncxml_name(root));
	stat = NC_ES3;
	goto done;
    }

    for(x=ncxml_child_first(root);x != NULL;x=ncxml_child_next(x)) {
	const char* elem = ncxml_name(x);
	if(strcmp(elem,"Prefix")==0) {
	    prefix = trim(ncxml_text(x),RECLAIM);
	    nclistpush(prefixes,prefix);
	    prefix = NULL;
	} else {
	    nclog(NCLOGERR,"Unexpected Element: <%s>",elem);
	    stat = NC_ES3;
	    goto done;
	}
    }

done:
    nullfree(prefix);
    return NCTHROW(stat);
}

static int
parse_checksumalgorithm(ncxml_t root, NClist* algorithms)
{
    int stat = NC_NOERR;
    char* alg = NULL;

    /* Verify top level element */
    if(strcmp(ncxml_name(root),"ChecksumAlgorithm")!=0) {
	nclog(NCLOGERR,"Expected: <ChecksumAlgorithm> actual: <%s>",ncxml_name(root));
	stat = NC_ES3;
	goto done;
    }
    alg = trim(ncxml_text(root),RECLAIM);
    nclistpush(algorithms,alg);
    alg = NULL;

done:
    nullfree(alg);
    return NCTHROW(stat);
}

static struct LISTOBJECTSV2*
alloclistobjectsv2(void)
{
    struct LISTOBJECTSV2* lov2 = NULL;
    if((lov2 = calloc(1,sizeof(struct LISTOBJECTSV2))) == NULL)
	return lov2;
    lov2->contents = nclistnew();
    lov2->commonprefixes = nclistnew();
    return lov2;
}

static struct Object*
allocobject(void)
{
    struct Object* obj = NULL;
    if((obj = calloc(1,sizeof(struct Object))) == NULL)
	return obj;
    obj->checksumalgorithms = nclistnew();
    return obj;
}

static void
reclaim_listobjectsv2(struct LISTOBJECTSV2* lo)
{
    int i;
    if(lo == NULL) return;
    nullfree(lo->istruncated);
    for(i=0;i<nclistlength(lo->contents);i++)
        reclaim_object((struct Object*)nclistget(lo->contents,i));
    nclistfree(lo->contents);
    nullfree(lo->name);
    nullfree(lo->prefix);
    nullfree(lo->delimiter);
    nullfree(lo->maxkeys);
    nclistfreeall(lo->commonprefixes);
    nullfree(lo->encodingtype);
    nullfree(lo->keycount);
    nullfree(lo->continuationtoken);
    nullfree(lo->nextcontinuationtoken);
    nullfree(lo->startafter);
    free(lo);
}

static void
reclaim_object(struct Object* o)
{
    if(o == NULL) return;
    nclistfreeall(o->checksumalgorithms);
    nullfree(o->etag);
    nullfree(o->key);
    nullfree(o->lastmodified);
        nullfree(o->owner.displayname);
        nullfree(o->owner.id);
    nullfree(o->size);
    nullfree(o->storageclass);
    free(o);
}

static char*
trim(char* s, int reclaim)
{
    ptrdiff_t first=0,last=0;
    const char* p;
    char* t = NULL;
    size_t len;
    
    for(p=s;*p;p++) {
	if(*p > ' ') {first = (p - s); break;}
    }
    for(p=s+(strlen(s)-1);p >= s;p--) {
	if(*p > ' ') {last = (p - s); break;}
    }
    len = (last - first) + 1;
    if((t = (char*)malloc(len+1))==NULL) return t;
    memcpy(t,s+first,len);
    t[len] = '\0';
    if(reclaim) nullfree(s);
    return t;
}

/*
Get Info about a single object from a vector
*/
static int
s3objectinfo1(const struct Object* s3_object, char** fullkeyp, uintptr_t* lenp)
{
    int stat = NC_NOERR;
    const char* key = NULL;
    char* tmp = NULL;
    unsigned long long len;

    assert(fullkeyp);

    key = s3_object->key;
    len = strlen(key);
    if((tmp = (char*)malloc(len+1+1))==NULL) {stat = NC_ENOMEM; goto done;}
    tmp[0] = '\0';
    if(key[0] != '/') strlcat(tmp,"/",len+1+1);
    strlcat(tmp,key,len+1+1);
    sscanf(s3_object->size,"%llu",&len);
    if(fullkeyp) {*fullkeyp = tmp; tmp = NULL;}
    if(lenp) *lenp = (size64_t)len;
done:
    if(tmp) free(tmp);
    return NCTHROW(stat);
}

/*
Get Info about a vector of objects; Keys are fixed up to start with a '/'
*/
static int
s3objectsinfo(NClist* contents, NClist* keys, NClist* lengths)
{
    int stat = NC_NOERR;
    size_t i;
    char* key = NULL;
    uintptr_t length = 0;

    for(i=0;i<nclistlength(contents);i++) {
        struct Object* s3_object = (struct Object*)nclistget(contents,i);
        if((stat = s3objectinfo1(s3_object,&key,&length))) goto done;
	if(keys != NULL) {nclistpush(keys,key);} else {nullfree(key);}
	key = NULL;
	if(lengths != NULL) nclistpush(lengths,(void*)length);	
    }
    if(keys != NULL) nclistnull(keys);
    if(lengths != NULL) nclistnull(lengths);
done:
    nullfree(key); /* avoid mem leak */
    return NCTHROW(stat);
}

/* Ensure each key starts with '/' */
static int
s3commonprefixes(NClist* list, NClist* keys)
{
    int stat = NC_NOERR;
    int i;

    for (i=0;i<nclistlength(list);i++) {
	char* p;
	size_t len;
	const char* prefix = (char*)nclistget(list,i);
        len = strlen(prefix);
        if((p = (char*) malloc(len+1+1))==NULL) /* for nul + leading '/' */
	    {stat = NC_ENOMEM; goto done;}
	*p = '\0';
	if(*prefix != '/') strlcat(p,"/",len+1+1);
	strlcat(p,prefix,len+1+1);
        nclistpush(keys,p); p = NULL;
    }
done:
    nclistnull(keys);
    return NCTHROW(stat);
}


/* Add new key,value pair to a query; encode before insertion; keep in sorted order */
static int
queryadd(NClist* query, const char* key, const char* value)
{
    int stat = NC_NOERR;
    char* ekey = NULL;
    char* evalue = NULL;

    if(key == NULL) {stat = NC_EINVAL; goto done;}
    if((stat = NCH5_s3comms_uriencode(&ekey, key, strlen(key), 1/*true*/, NULL))) goto done;
    evalue = NULL;
    if(value != NULL) {
	if((stat = NCH5_s3comms_uriencode(&evalue, value, strlen(value), 1/*true*/, NULL))) goto done;
    }
    /* Insert encoded key+value keeping sorted order */
    if((stat = queryinsert(query, ekey, evalue))) goto done;
    ekey = NULL;
    evalue = NULL;
done:
    nullfree(ekey);
    nullfree(evalue);    
    return NCTHROW(stat);
}

static int
queryend(NClist* query, char** querystring)
{
    int stat = NC_NOERR;
    size_t i;
    NCbytes* buf = ncbytesnew(); /* accumulate final query string */

    /* build the final query string */
    for(i=0;i<nclistlength(query);i+=2) {
        const char* key = (const char*)nclistget(query,i);
        const char* value = (const char*)nclistget(query,i+1);
	if(ncbyteslength(buf) > 0) ncbytescat(buf,"&");
	ncbytescat(buf,key);
        ncbytescat(buf,"=");
	if(value != NULL)
      	    ncbytescat(buf,value);
    }
    if(querystring) {*querystring = ncbytesextract(buf);}

    ncbytesfree(buf);
    return NCTHROW(stat);

}

/* Insert encoded key+value keeping sorted order */
static int 
queryinsert(NClist* list, char* ekey, char* evalue)
{
    int pos,i,stat = NC_NOERR;
    for(pos=-1,i=0;i<nclistlength(list);i+=2) {
	const char* key = (const char*)nclistget(list,i);
	int cmp = strcmp(key,ekey);
	if(cmp == 0) {stat = NC_EINVAL; goto done;} /* duplicate keys */
	if(cmp > 0) {pos = i; break;} /* key > ekey => insert ekey before key */
    }
    if(pos < 0) pos = nclistlength(list); /* insert at end; also works if |list|==0 */
    nclistinsert(list,pos,evalue);
    nclistinsert(list,pos,ekey);
done:
    return NCTHROW(stat);
}

