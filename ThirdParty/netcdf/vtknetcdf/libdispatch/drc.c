/*
Copyright (c) 1998-2018 University Corporation for Atmospheric Research/Unidata
See COPYRIGHT for license information.
*/

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "netcdf.h"
#include "ncbytes.h"
#include "ncuri.h"
#include "ncrc.h"
#include "nclog.h"
#include "ncauth.h"
#include "ncpathmgr.h"
#include "nc4internal.h"
#include "ncdispatch.h"

#ifndef nulldup
 #define nulldup(x) ((x)?strdup(x):(x))
#endif

#undef NOREAD

#undef DRCDEBUG
#undef LEXDEBUG
#undef PARSEDEBUG
#undef AWSDEBUG

#define RTAG ']'
#define LTAG '['

#undef MEMCHECK
#define MEMCHECK(x) if((x)==NULL) {goto nomem;} else {}

/* Alternate .aws directory location */
#define NC_TEST_AWS_DIR "NC_TEST_AWS_DIR"

/* Forward */
static int NC_rcload(void);
static char* rcreadline(char** nextlinep);
static void rctrim(char* text);
static void rcorder(NClist* rc);
static int rccompile(const char* path);
static int rcequal(NCRCentry* e1, NCRCentry* e2);
static int rclocatepos(const char* key, const char* hostport, const char* urlpath);
static struct NCRCentry* rclocate(const char* key, const char* hostport, const char* urlpath);
static int rcsearch(const char* prefix, const char* rcname, char** pathp);
static void rcfreeentries(NClist* rc);
static void rcfreeentry(NCRCentry* t);
#ifdef DRCDEBUG
static void storedump(char* msg, NClist* entrys);
#endif
static int aws_load_credentials(NCglobalstate*);
static void freeprofile(struct AWSprofile* profile);
static void freeprofilelist(NClist* profiles);

/* Define default rc files and aliases, also defines load order*/
static const char* rcfilenames[] = {".ncrc", ".daprc", ".dodsrc",NULL};

/* Read these files */
static const char* awsconfigfiles[] = {".aws/credentials",".aws/config",NULL};

static int NCRCinitialized = 0;

/**************************************************/
/* User API */

/**
The most common case is to get the most general value for a key,
where most general means that the urlpath and hostport are null
So this function returns the value associated with the key
where the .rc entry has the simple form "key=value".
If that entry is not found, then return NULL.

@param key table entry key field
@return value matching the key -- caller frees
@return NULL if no entry of the form key=value exists
*/
char*
nc_rc_get(const char* key)
{
    NCglobalstate* ncg = NULL;
    char* value = NULL;

    if(!NC_initialized) nc_initialize();

    ncg = NC_getglobalstate();
    assert(ncg != NULL && ncg->rcinfo != NULL && ncg->rcinfo->entries != NULL);
    if(ncg->rcinfo->ignore) return NC_NOERR;
    value = NC_rclookup(key,NULL,NULL);
    return nulldup(value);    
}

/**
Set simple key=value in .rc table.
Will overwrite any existing value.

@param key
@param value 
@return NC_NOERR if success
@return NC_EINVAL if fail
*/
int
nc_rc_set(const char* key, const char* value)
{
    int stat = NC_NOERR;
    NCglobalstate* ncg = NULL;

    if(!NC_initialized) nc_initialize();

    ncg = NC_getglobalstate();
    assert(ncg != NULL && ncg->rcinfo != NULL && ncg->rcinfo->entries != NULL);
    if(ncg->rcinfo->ignore) return NC_NOERR;
    stat = NC_rcfile_insert(key,NULL,NULL,value);
    return stat;
}

/**************************************************/
/* External Entry Points */

/*
Initialize defaults and load:
* .ncrc
* .daprc
* .dodsrc
* ${HOME}/.aws/config
* ${HOME}/.aws/credentials

For debugging support, it is possible
to change where the code looks for the .aws directory.
This is set by the environment variable NC_TEST_AWS_DIR.

*/

void
ncrc_initialize(void)
{
    int stat = NC_NOERR;
    NCglobalstate* ncg = NULL;

    if(NCRCinitialized) return;
    NCRCinitialized = 1; /* prevent recursion */

    ncg = NC_getglobalstate();

#ifndef NOREAD
    /* Load entrys */
    if((stat = NC_rcload())) {
        nclog(NCLOGWARN,".rc loading failed");
    }
    /* Load .aws/config */
    if((stat = aws_load_credentials(ncg))) {
        nclog(NCLOGWARN,"AWS config file not loaded");
    }
#endif
}

static void
ncrc_setrchome(void)
{
    const char* tmp = NULL;
    NCglobalstate* ncg = NC_getglobalstate();
    assert(ncg && ncg->home);
    if(ncg->rcinfo->rchome) return;
    tmp = getenv(NCRCENVHOME);
    if(tmp == NULL || strlen(tmp) == 0)
	tmp = ncg->home;
    ncg->rcinfo->rchome = strdup(tmp);
#ifdef DRCDEBUG
    fprintf(stderr,"ncrc_setrchome: %s\n",ncg->rcinfo->rchome);
#endif
}

void
NC_rcclear(NCRCinfo* info)
{
    if(info == NULL) return;
    nullfree(info->rcfile);
    nullfree(info->rchome);
    rcfreeentries(info->entries);
    freeprofilelist(info->s3profiles);

}

static void
rcfreeentry(NCRCentry* t)
{
	nullfree(t->host);
	nullfree(t->urlpath);
	nullfree(t->key);
	nullfree(t->value);
	free(t);
}

static void
rcfreeentries(NClist* rc)
{
    int i;
    for(i=0;i<nclistlength(rc);i++) {
	NCRCentry* t = (NCRCentry*)nclistget(rc,i);
	rcfreeentry(t);
    }
    nclistfree(rc);
}

/* locate, read and compile the rc files, if any */
static int
NC_rcload(void)
{
    int i,ret = NC_NOERR;
    char* path = NULL;
    NCglobalstate* globalstate = NULL;
    NClist* rcfileorder = nclistnew();

    if(!NCRCinitialized) ncrc_initialize();
    globalstate = NC_getglobalstate();

    if(globalstate->rcinfo->ignore) {
        nclog(NCLOGDBG,".rc file loading suppressed");
	goto done;
    }
    if(globalstate->rcinfo->loaded) goto done;

    /* locate the configuration files in order of use:
       1. Specified by NCRCENV_RC environment variable.
       2. If NCRCENV_RC is not set then merge the set of rc files in this order:
	  1. $RCHOME/.ncrc
  	  2. $RCHOME/.daprc
	  3. $RCHOME/.docsrc
	  4. $CWD/.ncrc
  	  5. $CWD/.daprc
	  6. $CWD/.docsrc
	  Entry in later files override any of the earlier files
    */
    if(globalstate->rcinfo->rcfile != NULL) { /* always use this */
	nclistpush(rcfileorder,strdup(globalstate->rcinfo->rcfile));
    } else {
	const char** rcname;
	const char* dirnames[3];
	const char** dir;

        /* Make sure rcinfo.rchome is defined */
	ncrc_setrchome();
	dirnames[0] = globalstate->rcinfo->rchome;
	dirnames[1] = globalstate->cwd;
	dirnames[2] = NULL;

        for(dir=dirnames;*dir;dir++) {
	    for(rcname=rcfilenames;*rcname;rcname++) {
	        ret = rcsearch(*dir,*rcname,&path);
		if(ret == NC_NOERR && path != NULL)
		    nclistpush(rcfileorder,path);
		path = NULL;
	    }
	}
    }
    for(i=0;i<nclistlength(rcfileorder);i++) {
	path = (char*)nclistget(rcfileorder,i);
	if((ret=rccompile(path))) {
	    nclog(NCLOGWARN, "Error parsing %s\n",path);
	    ret = NC_NOERR; /* ignore it */
	    goto done;
	}
    }

done:
    globalstate->rcinfo->loaded = 1; /* even if not exists */
    nclistfreeall(rcfileorder);
    return (ret);
}

/**
 * Locate a entry by property key and host+port (may be null)
 * If duplicate keys, first takes precedence.
 */
char*
NC_rclookup(const char* key, const char* hostport, const char* urlpath)
{
    struct NCRCentry* entry = NULL;
    if(!NCRCinitialized) ncrc_initialize();
    entry = rclocate(key,hostport,urlpath);
    return (entry == NULL ? NULL : entry->value);
}

/**
 * Locate a entry by property key and uri.
 * If duplicate keys, first takes precedence.
 */
char*
NC_rclookupx(NCURI* uri, const char* key)
{
    char* hostport = NULL;
    char* result = NULL;

    hostport = NC_combinehostport(uri);
    result = NC_rclookup(key,hostport,uri->path);
    nullfree(hostport);
    return result;
}

#if 0
/*!
Set the absolute path to use for the rc file.
WARNING: this MUST be called before any other
call in order for this to take effect.

\param[in] rcfile The path to use. If NULL then do not use any rcfile.

\retval OC_NOERR if the request succeeded.
\retval OC_ERCFILE if the file failed to load
*/

int
NC_set_rcfile(const char* rcfile)
{
    int stat = NC_NOERR;
    FILE* f = NULL;
    NCglobalstate* globalstate = NC_getglobalstate();

    if(rcfile != NULL && strlen(rcfile) == 0)
	rcfile = NULL;
    f = NCfopen(rcfile,"r");
    if(f == NULL) {
	stat = NC_ERCFILE;
        goto done;
    }
    fclose(f);
    nullfree(globalstate->rcinfo->rcfile);
    globalstate->rcinfo->rcfile = strdup(rcfile);
    /* Clear globalstate->rcinfo */
    NC_rcclear(&globalstate->rcinfo);
    /* (re) load the rcfile and esp the entriestore*/
    stat = NC_rcload();
done:
    return stat;
}
#endif

/**************************************************/
/* RC processing functions */

static char*
rcreadline(char** nextlinep)
{
    char* line;
    char* p;

    line = (p = *nextlinep);
    if(*p == '\0') return NULL; /*signal done*/
    for(;*p;p++) {
	if(*p == '\r' && p[1] == '\n') *p = '\0';
	else if(*p == '\n') break;
    }
    *p++ = '\0'; /* null terminate line; overwrite newline */
    *nextlinep = p;
    return line;
}

/* Trim TRIMCHARS from both ends of text; */
static void
rctrim(char* text)
{
    char* p;
    char* q;
    size_t len = 0;
    int i;

    if(text == NULL || *text == '\0') return;

    len = strlen(text);

    /* elide upto first non-trimchar */
    for(q=text,p=text;*p;p++) {
	if(*p != ' ' && *p != '\t' && *p != '\r') {*q++ = *p;}
    }
    len = strlen(p);
    /* locate last non-trimchar */
    if(len > 0) {
        for(i=(len-1);i>=0;i--) {
	    p = &text[i];
	    if(*p != ' ' && *p != '\t' && *p != '\r') {break;}
	    *p = '\0'; /* elide trailing trimchars */
        }
    }
}

/* Order the entries: those with urls must be first,
   but otherwise relative order does not matter.
*/
static void
rcorder(NClist* rc)
{
    int i;
    int len = nclistlength(rc);
    NClist* tmprc = NULL;
    if(rc == NULL || len == 0) return;
    tmprc = nclistnew();
    /* Two passes: 1) pull entries with host */
    for(i=0;i<len;i++) {
        NCRCentry* ti = nclistget(rc,i);
	if(ti->host == NULL) continue;
	nclistpush(tmprc,ti);
    }
    /* pass 2 pull entries without host*/
    for(i=0;i<len;i++) {
        NCRCentry* ti = nclistget(rc,i);
	if(ti->host != NULL) continue;
	nclistpush(tmprc,ti);
    }
    /* Move tmp to rc */
    nclistsetlength(rc,0);
    for(i=0;i<len;i++) {
        NCRCentry* ti = nclistget(tmprc,i);
	nclistpush(rc,ti);
    }
#ifdef DRCDEBUG
    storedump("reorder:",rc);
#endif
    nclistfree(tmprc);
}

/* Merge a entry store from a file*/
static int
rccompile(const char* filepath)
{
    int ret = NC_NOERR;
    NClist* rc = NULL;
    char* contents = NULL;
    NCbytes* tmp = ncbytesnew();
    NCURI* uri = NULL;
    char* nextline = NULL;
    NCglobalstate* globalstate = NC_getglobalstate();
    char* bucket = NULL;

    if((ret=NC_readfile(filepath,tmp))) {
        nclog(NCLOGWARN, "Could not open configuration file: %s",filepath);
	goto done;
    }
    contents = ncbytesextract(tmp);
    if(contents == NULL) contents = strdup("");
    /* Either reuse or create new  */
    rc = globalstate->rcinfo->entries;
    if(rc == NULL) {
        rc = nclistnew();
        globalstate->rcinfo->entries = rc;
    }
    nextline = contents;
    for(;;) {
	char* line;
	char* key = NULL;
        char* value = NULL;
        char* host = NULL;
        char* urlpath = NULL;
	size_t llen;
        NCRCentry* entry;

	line = rcreadline(&nextline);
	if(line == NULL) break; /* done */
        rctrim(line);  /* trim leading and trailing blanks */
        if(line[0] == '#') continue; /* comment */
	if((llen=strlen(line)) == 0) continue; /* empty line */
	if(line[0] == LTAG) {
	    char* url = ++line;
            char* rtag = strchr(line,RTAG);
            if(rtag == NULL) {
                nclog(NCLOGERR, "Malformed [url] in %s entry: %s",filepath,line);
		continue;
            }
            line = rtag + 1;
            *rtag = '\0';
            /* compile the url and pull out the host and protocol */
            if(uri) ncurifree(uri);
            if(ncuriparse(url,&uri)) {
                nclog(NCLOGERR, "Malformed [url] in %s entry: %s",filepath,line);
		continue;
            }
	    if(NC_iss3(uri)) {
	         NCURI* newuri = NULL;
	        /* Rebuild the url to S3 "path" format */
	        nullfree(bucket);
	        if((ret = NC_s3urlrebuild(uri,&newuri,&bucket,NULL))) goto done;
		ncurifree(uri);
		uri = newuri;
		newuri = NULL;
	    }
	    /* Get the host+port */
            ncbytesclear(tmp);
            ncbytescat(tmp,uri->host);
            if(uri->port != NULL) {
		ncbytesappend(tmp,':');
                ncbytescat(tmp,uri->port);
            }
            ncbytesnull(tmp);
            host = ncbytesextract(tmp);
	    if(strlen(host)==0) /* nullify host */
		{free(host); host = NULL;}
	    /* Get the url path part */
	    urlpath = uri->path;
	    if(urlpath && strlen(urlpath)==0) urlpath = NULL; /* nullify */
	}
        /* split off key and value */
        key=line;
        value = strchr(line, '=');
        if(value == NULL)
            value = line + strlen(line);
        else {
            *value = '\0';
            value++;
        }
	/* See if key already exists */
	entry = rclocate(key,host,urlpath);
	if(entry == NULL) {
	    entry = (NCRCentry*)calloc(1,sizeof(NCRCentry));
	    if(entry == NULL) {ret = NC_ENOMEM; goto done;}
	    nclistpush(rc,entry);
	    entry->host = host; host = NULL;
	    entry->urlpath = nulldup(urlpath);
    	    entry->key = nulldup(key);
            rctrim(entry->host);
            rctrim(entry->urlpath);
            rctrim(entry->key);
	}
	nullfree(entry->value);
        entry->value = nulldup(value);
        rctrim(entry->value);

#ifdef DRCDEBUG
	fprintf(stderr,"rc: host=%s urlpath=%s key=%s value=%s\n",
		(entry->host != NULL ? entry->host : "<null>"),
		(entry->urlpath != NULL ? entry->urlpath : "<null>"),
		entry->key,entry->value);
#endif
	entry = NULL;
    }
#ifdef DRCDEBUG
    fprintf(stderr,"reorder.path=%s\n",filepath);
#endif
    rcorder(rc);

done:
    if(contents) free(contents);
    ncurifree(uri);
    ncbytesfree(tmp);
    return (ret);
}

/**
Encapsulate equality comparison: return 1|0
*/
static int
rcequal(NCRCentry* e1, NCRCentry* e2)
{
    int nulltest;
    if(e1->key == NULL || e2->key == NULL) return 0;
    if(strcmp(e1->key,e2->key) != 0) return 0;
    /* test hostport; take NULL into account*/
    nulltest = 0;
    if(e1->host == NULL) nulltest |= 1;
    if(e2->host == NULL) nulltest |= 2;
    switch (nulltest) {
    case 0: if(strcmp(e1->host,e2->host) != 0) {return 0;}  break;
    case 1: return 0;
    case 2: return 0;
    case 3: break;
    default: return 0;
    }
    /* test urlpath take NULL into account*/
    nulltest = 0;
    if(e1->urlpath == NULL) nulltest |= 1;
    if(e2->urlpath == NULL) nulltest |= 2;
    switch (nulltest) {
    case 0: if(strcmp(e1->urlpath,e2->urlpath) != 0) {return 0;} break;
    case 1: return 0;
    case 2: return 0;
    case 3: break;
    default: return 0;
    }
    return 1;
}

/**
 * (Internal) Locate a entry by property key and host+port (may be null) and urlpath (may be null)
 * If duplicate keys, first takes precedence.
 */
static int
rclocatepos(const char* key, const char* hostport, const char* urlpath)
{
    int i;
    NCglobalstate* globalstate = NC_getglobalstate();
    struct NCRCinfo* info = globalstate->rcinfo;
    NCRCentry* entry = NULL;
    NCRCentry candidate;
    NClist* rc = info->entries;

    if(info->ignore) return -1;

    candidate.key = (char*)key;
    candidate.value = (char*)NULL;
    candidate.host = (char*)hostport;
    candidate.urlpath = (char*)urlpath;

    for(i=0;i<nclistlength(rc);i++) {
      entry = (NCRCentry*)nclistget(rc,i);
      if(rcequal(entry,&candidate)) return i;
    }
    return -1;
}

/**
 * (Internal) Locate a entry by property key and host+port (may be null or "").
 * If duplicate keys, first takes precedence.
 */
static struct NCRCentry*
rclocate(const char* key, const char* hostport, const char* urlpath)
{
    int pos;
    NCglobalstate* globalstate = NC_getglobalstate();
    struct NCRCinfo* info = globalstate->rcinfo;

    if(globalstate->rcinfo->ignore) return NULL;
    if(key == NULL || info == NULL) return NULL;
    pos = rclocatepos(key,hostport,urlpath);
    if(pos < 0) return NULL;
    return NC_rcfile_ith(info,(size_t)pos);
}

/**
 * Locate rc file by searching in directory prefix.
 */
static
int
rcsearch(const char* prefix, const char* rcname, char** pathp)
{
    char* path = NULL;
    FILE* f = NULL;
    size_t plen = (prefix?strlen(prefix):0);
    size_t rclen = strlen(rcname);
    int ret = NC_NOERR;

    size_t pathlen = plen+rclen+1+1; /*+1 for '/' +1 for nul */
    path = (char*)malloc(pathlen); /* +1 for nul*/
    if(path == NULL) {ret = NC_ENOMEM;	goto done;}
    snprintf(path, pathlen, "%s/%s", prefix, rcname);
    /* see if file is readable */
    f = NCfopen(path,"r");
    if(f != NULL)
        nclog(NCLOGDBG, "Found rc file=%s",path);
done:
    if(f == NULL || ret != NC_NOERR) {
	nullfree(path);
	path = NULL;
    }
    if(f != NULL)
      fclose(f);
    if(pathp != NULL)
      *pathp = path;
    else {
      nullfree(path);
      path = NULL;
    }
    errno = 0; /* silently ignore errors */
    return (ret);
}

int
NC_rcfile_insert(const char* key, const char* hostport, const char* urlpath, const char* value)
{
    int ret = NC_NOERR;
    /* See if this key already defined */
    struct NCRCentry* entry = NULL;
    NCglobalstate* globalstate = NULL;
    NClist* rc = NULL;

    if(!NCRCinitialized) ncrc_initialize();

    if(key == NULL || value == NULL)
        {ret = NC_EINVAL; goto done;}

    globalstate = NC_getglobalstate();
    rc = globalstate->rcinfo->entries;

    if(rc == NULL) {
	rc = nclistnew();
        globalstate->rcinfo->entries = rc;
	if(rc == NULL) {ret = NC_ENOMEM; goto done;}
    }
    entry = rclocate(key,hostport,urlpath);
    if(entry == NULL) {
	entry = (NCRCentry*)calloc(1,sizeof(NCRCentry));
	if(entry == NULL) {ret = NC_ENOMEM; goto done;}
	entry->key = strdup(key);
	entry->value = NULL;
        rctrim(entry->key);
        entry->host = nulldup(hostport);
        rctrim(entry->host);
        entry->urlpath = nulldup(urlpath);
        rctrim(entry->urlpath);
	nclistpush(rc,entry);
    }
    if(entry->value != NULL) free(entry->value);
    entry->value = strdup(value);
    rctrim(entry->value);
#ifdef DRCDEBUG
    storedump("NC_rcfile_insert",rc);
#endif    
done:
    return ret;
}

/* Obtain the count of number of entries */
size_t
NC_rcfile_length(NCRCinfo* info)
{
    return nclistlength(info->entries);
}

/* Obtain the ith entry; return NULL if out of range */
NCRCentry*
NC_rcfile_ith(NCRCinfo* info, size_t i)
{
    if(i >= nclistlength(info->entries))
	return NULL;
    return (NCRCentry*)nclistget(info->entries,i);
}


#ifdef DRCDEBUG
static void
storedump(char* msg, NClist* entries)
{
    int i;

    if(msg != NULL) fprintf(stderr,"%s\n",msg);
    if(entries == NULL || nclistlength(entries)==0) {
        fprintf(stderr,"<EMPTY>\n");
        return;
    }
    for(i=0;i<nclistlength(entries);i++) {
	NCRCentry* t = (NCRCentry*)nclistget(entries,i);
        fprintf(stderr,"\t%s\t%s\t%s\n",
                ((t->host == NULL || strlen(t->host)==0)?"--":t->host),t->key,t->value);
    }
    fflush(stderr);
}
#endif

/**************************************************/
/*
Get the current active profile. The priority order is as follows:
1. aws.profile key in mode flags
2. aws.profile in .rc entries
4. "default"

@param uri uri with mode flags, may be NULL
@param profilep return profile name here or NULL if none found
@return NC_NOERR if no error.
@return NC_EINVAL if something else went wrong.
*/

int
NC_getactives3profile(NCURI* uri, const char** profilep)
{
    int stat = NC_NOERR;
    const char* profile = NULL;

    profile = ncurifragmentlookup(uri,"aws.profile");
    if(profile == NULL)
        profile = NC_rclookupx(uri,"AWS.PROFILE");
    if(profile == NULL)
        profile = "default";
#ifdef AWSDEBUG
    fprintf(stderr,">>> activeprofile = %s\n",(profile?profile:"null"));
#endif
    if(profilep) *profilep = profile;
    return stat;
}

/*
Get the current default region. The search order is as follows:
1. aws.region key in mode flags
2. aws.region in .rc entries
3. aws_region key in current profile (only if profiles are being used)
4. "us-east-1"

@param uri uri with mode flags, may be NULL
@param regionp return region name here or NULL if none found
@return NC_NOERR if no error.
@return NC_EINVAL if something else went wrong.
*/

int
NC_getdefaults3region(NCURI* uri, const char** regionp)
{
    int stat = NC_NOERR;
    const char* region = NULL;
    const char* profile = NULL;

    region = ncurifragmentlookup(uri,"aws.region");
    if(region == NULL)
        region = NC_rclookupx(uri,"AWS.REGION");
    if(region == NULL) {/* See if we can find a profile */
        if((stat = NC_getactives3profile(uri,&profile))==NC_NOERR) {
	    if(profile)
	        (void)NC_s3profilelookup(profile,"aws_region",&region);
	}
    }
    if(region == NULL)
        region = "us-east-1";
#ifdef AWSDEBUG
    fprintf(stderr,">>> activeregion = %s\n",(region?region:"null"));
#endif
    if(regionp) *regionp = region;
    return stat;
}

/**
The .aws/config and .aws/credentials files
are in INI format (https://en.wikipedia.org/wiki/INI_file).
This format is not well defined, so the grammar used
here is restrictive. Here, the term "profile" is the same
as the INI term "section".

The grammar used is as follows:

Grammar:

inifile: profilelist ;
profilelist: profile | profilelist profile ;
profile: '[' profilename ']' EOL entries ;
entries: empty | entries entry ;
entry:  WORD = WORD EOL ;
profilename: WORD ;
Lexical:
WORD    sequence of printable characters - [ \[\]=]+
EOL	'\n' | ';'

Note:
1. The semicolon at beginning of a line signals a comment.
2. # comments are not allowed
3. Duplicate profiles or keys are ignored.
4. Escape characters are not supported.
*/

#define AWS_EOF (-1)
#define AWS_ERR (0)
#define AWS_WORD (0x10001)
#define AWS_EOL (0x10002)

#ifdef LEXDEBUG
static const char*
tokenname(int token)
{
    static char num[32];
    switch(token) {
    case AWS_EOF: return "EOF";
    case AWS_ERR: return "ERR";
    case AWS_WORD: return "WORD";
    default: snprintf(num,sizeof(num),"%d",token); return num;
    }
    return "UNKNOWN";
}
#endif

typedef struct AWSparser {
    char* text;
    char* pos;
    size_t yylen; /* |yytext| */
    NCbytes* yytext;
    int token; /* last token found */
    int pushback; /* allow 1-token pushback */
} AWSparser;

static int
awslex(AWSparser* parser)
{
    int c;
    int token = 0;
    char* start;
    size_t count;

    parser->token = AWS_ERR;
    ncbytesclear(parser->yytext);
    ncbytesnull(parser->yytext);

    if(parser->pushback != AWS_ERR) {
	token = parser->pushback;
	parser->pushback = AWS_ERR;
	goto done;
    }

    while(token == 0) { /* avoid need to goto when retrying */
	c = *parser->pos;
	if(c == '\0') {
	    token = AWS_EOF;
	} else if(c == '\n') {
	    parser->pos++;
	    token = AWS_EOL;
	} else if(c <= ' ' || c == '\177') {
	    parser->pos++;
	    continue; /* ignore whitespace */
	} else if(c == ';') {
	    char* p = parser->pos - 1;
	    if(*p == '\n') {
	        /* Skip comment */
	        do {p++;} while(*p != '\n' && *p != '\0');
	        parser->pos = p;
	        token = (*p == '\n'?AWS_EOL:AWS_EOF);
	    } else {
	        token = ';';
	        ncbytesappend(parser->yytext,';');
		parser->pos++;
	    }
	} else if(c == '[' || c == ']' || c == '=') {
	    ncbytesappend(parser->yytext,c);
    	    ncbytesnull(parser->yytext);
	    token = c;
	    parser->pos++;
	} else { /*Assume a word*/
	    start = parser->pos;
	    for(;;) {
		c = *parser->pos++;
	        if(c <= ' ' || c == '\177' || c == '[' || c == ']' || c == '=') break; /* end of word */
	    }
	    /* Pushback last char */
	    parser->pos--;
	    count = ((parser->pos) - start);
	    ncbytesappendn(parser->yytext,start,count);
	    ncbytesnull(parser->yytext);
	    token = AWS_WORD;
	}
#ifdef LEXDEBUG
fprintf(stderr,"%s(%d): |%s|\n",tokenname(token),token,ncbytescontents(parser->yytext));
#endif
    } /*for(;;)*/

done:
    parser->token = token;
    return token;
}

/*
@param text of the aws credentials file
@param profiles list of form struct AWSprofile (see ncauth.h)
*/

#define LBR '['
#define RBR ']'

static int
awsparse(const char* text, NClist* profiles)
{
    int i,stat = NC_NOERR;
    size_t len;
    AWSparser* parser = NULL;
    struct AWSprofile* profile = NULL;
    int token;
    char* key = NULL;
    char* value = NULL;

    if(text == NULL) text = "";

    parser = calloc(1,sizeof(AWSparser));
    if(parser == NULL)
	{stat = (NC_ENOMEM); goto done;}
    len = strlen(text);
    parser->text = (char*)malloc(len+1+1+1); /* double nul term plus leading EOL */
    if(parser->text == NULL)
	{stat = (NCTHROW(NC_EINVAL)); goto done;}
    parser->pos = parser->text;
    parser->pos[0] = '\n'; /* So we can test for comment unconditionally */
    parser->pos++;
    strcpy(parser->text+1,text);
    parser->pos += len;
    /* Double nul terminate */
    parser->pos[0] = '\0';
    parser->pos[1] = '\0';
    parser->pos = &parser->text[0]; /* reset */
    parser->yytext = ncbytesnew();
    parser->pushback = AWS_ERR;

    /* Do not need recursion, use simple loops */
    for(;;) {
        token = awslex(parser); /* make token always be defined */
	if(token ==  AWS_EOF) break; /* finished */
	if(token ==  AWS_EOL) {continue;} /* blank line */
	if(token != LBR) {stat = NCTHROW(NC_EINVAL); goto done;}
	/* parse [profile name] */
        token = awslex(parser);
	if(token != AWS_WORD) {stat = NCTHROW(NC_EINVAL); goto done;}
	assert(profile == NULL);
	if((profile = (struct AWSprofile*)calloc(1,sizeof(struct AWSprofile)))==NULL)
	    {stat = NC_ENOMEM; goto done;}
	profile->name = ncbytesextract(parser->yytext);
	profile->entries = nclistnew();
        token = awslex(parser);
	if(token != RBR) {stat = NCTHROW(NC_EINVAL); goto done;}
#ifdef PARSEDEBUG
fprintf(stderr,">>> parse: profile=%s\n",profile->name);
#endif
	/* The fields can be in any order */
	for(;;) {
	    struct AWSentry* entry = NULL;
            token = awslex(parser);
	    if(token == AWS_EOL) {
	        continue; /* ignore empty lines */
	    } else if(token == AWS_EOF) {
	        break;
	    } else if(token == LBR) {/* start of next profile */
	        parser->pushback = token;
		break;
	    } else if(token ==  AWS_WORD) {
	    	key = ncbytesextract(parser->yytext);
		token = awslex(parser);
	        if(token != '=') {stat = NCTHROW(NC_EINVAL); goto done;}
	        token = awslex(parser);
		if(token != AWS_EOL && token != AWS_WORD) {stat = NCTHROW(NC_EINVAL); goto done;}
	        value = ncbytesextract(parser->yytext);
	        if((entry = (struct AWSentry*)calloc(1,sizeof(struct AWSentry)))==NULL)
	            {stat = NC_ENOMEM; goto done;}
	        entry->key = key; key = NULL;
    	        entry->value = value; value = NULL;
#ifdef PARSEDEBUG
fprintf(stderr,">>> parse: entry=(%s,%s)\n",entry->key,entry->value);
#endif
		nclistpush(profile->entries,entry); entry = NULL;
		if(token == AWS_WORD) token = awslex(parser); /* finish the line */
	    } else
	        {stat = NCTHROW(NC_EINVAL); goto done;}
	}

	/* If this profile already exists, then ignore new one */
	for(i=0;i<nclistlength(profiles);i++) {
	    struct AWSprofile* p = (struct AWSprofile*)nclistget(profiles,i);
	    if(strcasecmp(p->name,profile->name)==0) {
		/* reclaim and ignore */
		freeprofile(profile);
		profile = NULL;
		break;
	    }
	}
	if(profile) nclistpush(profiles,profile);
	profile = NULL;
    }

done:
    if(profile) freeprofile(profile);
    nullfree(key);
    nullfree(value);
    if(parser != NULL) {
	nullfree(parser->text);
	ncbytesfree(parser->yytext);
	free(parser);
    }
    return (stat);
}

static void
freeentry(struct AWSentry* e)
{
    if(e) {
#ifdef AWSDEBUG
fprintf(stderr,">>> freeentry: key=%s value=%s\n",e->key,e->value);
#endif
        nullfree(e->key);
        nullfree(e->value);
        nullfree(e);
    }
}

static void
freeprofile(struct AWSprofile* profile)
{
    if(profile) {
	int i;
#ifdef AWSDEBUG
fprintf(stderr,">>> freeprofile: %s\n",profile->name);
#endif
	for(i=0;i<nclistlength(profile->entries);i++) {
	    struct AWSentry* e = (struct AWSentry*)nclistget(profile->entries,i);
	    freeentry(e);
	}
        nclistfree(profile->entries);
	nullfree(profile->name);
	nullfree(profile);
    }
}

static void
freeprofilelist(NClist* profiles)
{
    if(profiles) {
	int i;
	for(i=0;i<nclistlength(profiles);i++) {
	    struct AWSprofile* p = (struct AWSprofile*)nclistget(profiles,i);
	    freeprofile(p);
	}
	nclistfree(profiles);
    }
}

/* Find, load, and parse the aws credentials file */
static int
aws_load_credentials(NCglobalstate* gstate)
{
    int stat = NC_NOERR;
    NClist* profiles = nclistnew();
    const char** awscfg = awsconfigfiles;
    const char* aws_root = getenv(NC_TEST_AWS_DIR);
    NCbytes* buf = ncbytesnew();
    char path[8192];

    for(;*awscfg;awscfg++) {
        /* Construct the path ${HOME}/<file> or Windows equivalent. */
	const char* cfg = *awscfg;

        snprintf(path,sizeof(path),"%s%s%s",
	    (aws_root?aws_root:gstate->home),
	    (*cfg == '/'?"":"/"),
	    cfg);
	ncbytesclear(buf);
        if((stat=NC_readfile(path,buf))) {
            nclog(NCLOGWARN, "Could not open file: %s",path);
        } else {
            /* Parse the credentials file */
	    const char* text = ncbytescontents(buf);
            if((stat = awsparse(text,profiles))) goto done;
	}
    }

    /* add a "none" credentials */
    {
	struct AWSprofile* noprof = (struct AWSprofile*)calloc(1,sizeof(struct AWSprofile));
	noprof->name = strdup("none");
	noprof->entries = nclistnew();
	nclistpush(profiles,noprof); noprof = NULL;
    }

    if(gstate->rcinfo->s3profiles)
        freeprofilelist(gstate->rcinfo->s3profiles);
    gstate->rcinfo->s3profiles = profiles; profiles = NULL;

#ifdef AWSDEBUG
    {int i,j;
	fprintf(stderr,">>> profiles:\n");
	for(i=0;i<nclistlength(creds->profiles);i++) {
	    struct AWSprofile* p = (struct AWSprofile*)nclistget(creds->profiles,i);
	    fprintf(stderr,"    [%s]",p->name);
	    for(j=0;j<nclistlength(p->entries);j++) {
	        struct AWSentry* e = (struct AWSentry*)nclistget(p->entries,j);
		fprintf(stderr," %s=%s",e->key,e->value);
	    }
            fprintf(stderr,"\n");
	}
    }
#endif

done:
    ncbytesfree(buf);
    freeprofilelist(profiles);
    return stat;
}

int
NC_authgets3profile(const char* profilename, struct AWSprofile** profilep)
{
    int stat = NC_NOERR;
    int i = -1;
    NCglobalstate* gstate = NC_getglobalstate();

    for(i=0;i<nclistlength(gstate->rcinfo->s3profiles);i++) {
	struct AWSprofile* profile = (struct AWSprofile*)nclistget(gstate->rcinfo->s3profiles,i);
	if(strcmp(profilename,profile->name)==0)
	    {if(profilep) {*profilep = profile; goto done;}}
    }
    if(profilep) *profilep = NULL; /* not found */
done:
    return stat;
}

int
NC_s3profilelookup(const char* profile, const char* key, const char** valuep)
{
    int i,stat = NC_NOERR;
    struct AWSprofile* awsprof = NULL;
    const char* value = NULL;

    if(profile == NULL) return NC_ES3;
    stat = NC_authgets3profile(profile,&awsprof);
    if(stat == NC_NOERR && awsprof != NULL) {
        for(i=0;i<nclistlength(awsprof->entries);i++) {
	    struct AWSentry* entry = (struct AWSentry*)nclistget(awsprof->entries,i);
	    if(strcasecmp(entry->key,key)==0) {
		value = entry->value;
	        break;
	    }
	}
    }
    if(valuep) *valuep = value;
    return stat;
}
