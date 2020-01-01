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
#include "netcdf.h"
#include "ncbytes.h"
#include "ncuri.h"
#include "ncrc.h"
#include "nclog.h"
#include "ncwinpath.h"

#define RCFILEENV "DAPRCFILE"

#define RTAG ']'
#define LTAG '['

#define TRIMCHARS " \t\r\n"

#undef MEMCHECK
#define MEMCHECK(x) if((x)==NULL) {goto nomem;} else {}

/* Forward */
static char* rcreadline(char** nextlinep);
static void rctrim(char* text);
static void rcorder(NClist* rc);
static int rccompile(const char* path);
static struct NCTriple* rclocate(const char* key, const char* hostport);
static int rcsearch(const char* prefix, const char* rcname, char** pathp);
static void rcfreetriples(NClist* rc);
#ifdef D4DEBUG
static void storedump(char* msg, NClist* triples);
#endif

/* Define default rc files and aliases, also defines search order*/
static const char* rcfilenames[] = {".daprc",".dodsrc",".ncrc",NULL};

/**************************************************/
/* External Entry Points */

static NCRCglobalstate* ncrc_globalstate = NULL;

/* Get global state */
NCRCglobalstate*
ncrc_getglobalstate(void)
{
    if(ncrc_globalstate == NULL) {
        ncrc_globalstate = calloc(1,sizeof(NCRCglobalstate));
    }
    return ncrc_globalstate;
}

void
ncrc_freeglobalstate(void)
{
    if(ncrc_globalstate != NULL) {
        nullfree(ncrc_globalstate->tempdir);
        nullfree(ncrc_globalstate->home);
        NC_rcclear(&ncrc_globalstate->rcinfo);
	free(ncrc_globalstate);
	ncrc_globalstate = NULL;
    }
}

void
NC_rcclear(NCRCinfo* info)
{
    if(info == NULL) return;
    nullfree(info->rcfile);
    rcfreetriples(info->triples);
}

void
rcfreetriples(NClist* rc)
{
    int i;
    for(i=0;i<nclistlength(rc);i++) {
	NCTriple* t = (NCTriple*)nclistget(rc,i);
	nullfree(t->host);
	nullfree(t->key);
	nullfree(t->value);
	free(t);
    }
    nclistfree(rc);
}

/* locate, read and compile the rc file, if any */
int
NC_rcload(void)
{
    int ret = NC_NOERR;
    char* path = NULL;
    NCRCglobalstate* globalstate = ncrc_getglobalstate();

    if(globalstate->rcinfo.ignore) {
        nclog(NCLOGDBG,"No runtime configuration file specified; continuing");
	return (NC_NOERR);
    }
    if(globalstate->rcinfo.loaded) return (NC_NOERR);

    /* locate the configuration files in the following order:
       1. specified by NC_set_rcfile
       2. set by DAPRCFILE env variable
       3. ./<rcfile> (current directory)
       4. $HOME/<rcfile>
    */
    if(globalstate->rcinfo.rcfile != NULL) { /* always use this */
	path = strdup(globalstate->rcinfo.rcfile);
    } else if(getenv(RCFILEENV) != NULL && strlen(getenv(RCFILEENV)) > 0) {
        path = strdup(getenv(RCFILEENV));
    } else {
	const char** rcname;
	int found = 0;
	for(rcname=rcfilenames;!found && *rcname;rcname++) {
	    ret = rcsearch(".",*rcname,&path);
    	    if(ret == NC_NOERR && path == NULL)  /* try $HOME */
	        ret = rcsearch(globalstate->home,*rcname,&path);
	    if(ret != NC_NOERR)
		goto done;
	    if(path != NULL)
		found = 1;
	}
    }
    if(path == NULL) {
        nclog(NCLOGDBG,"Cannot find runtime configuration file; continuing");
    } else {
#ifdef D4DEBUG
        fprintf(stderr, "RC file: %s\n", path);
#endif
        if((ret=rccompile(path))) {
	    nclog(NCLOGERR, "Error parsing %s\n",path);
	    goto done;
	}
    }
done:
    globalstate->rcinfo.loaded = 1; /* even if not exists */
    nullfree(path);
    return (ret);
}

/**
 * Locate a triple by property key and host+port (may be null|"")
 * If duplicate keys, first takes precedence.
 */
char*
NC_rclookup(const char* key, const char* hostport)
{
    struct NCTriple* triple = rclocate(key,hostport);
    return (triple == NULL ? NULL : triple->value);
}

/*!
Set the absolute path to use for the rc file.
WARNING: this MUST be called before any other
call in order for this to take effect.

\param[in] rcfile The path to use. If NULL, or "",
                  then do not use any rcfile.

\retval OC_NOERR if the request succeeded.
\retval OC_ERCFILE if the file failed to load
*/

int
NC_set_rcfile(const char* rcfile)
{
    int stat = NC_NOERR;
    FILE* f = NULL;
    NCRCglobalstate* globalstate = ncrc_getglobalstate();

    if(rcfile != NULL && strlen(rcfile) == 0)
	rcfile = NULL;
    f = NCfopen(rcfile,"r");
    if(f == NULL) {
	stat = NC_ERCFILE;
        goto done;
    }
    fclose(f);
    nullfree(globalstate->rcinfo.rcfile);
    globalstate->rcinfo.rcfile = strdup(rcfile);
    /* Clear globalstate->rcinfo */
    NC_rcclear(&globalstate->rcinfo);
    /* (re) load the rcfile and esp the triplestore*/
    stat = NC_rcload();
done:
    return stat;
}

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
    char* p = text;
    size_t len = 0;
    int i;

    /* locate first non-trimchar */
    for(;*p;p++) {
       if(strchr(TRIMCHARS,*p) == NULL) break; /* hit non-trim char */
    }
    memmove(text,p,strlen(p)+1);
    len = strlen(text);
    /* locate last non-trimchar */
    if(len > 0) {
        for(i=(len-1);i>=0;i--) {
            if(strchr(TRIMCHARS,text[i]) == NULL) {
                text[i+1] = '\0'; /* elide trailing trimchars */
                break;
            }
        }
    }
}

/* Order the triples: those with urls must be first,
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
    /* Copy rc into tmprc and clear rc */
    for(i=0;i<len;i++) {
        NCTriple* ti = nclistget(rc,i);
        nclistpush(tmprc,ti);
    }
    nclistclear(rc);
    /* Two passes: 1) pull triples with host */
    for(i=0;i<len;i++) {
        NCTriple* ti = nclistget(tmprc,i);
	if(ti->host == NULL) continue;
	nclistpush(rc,ti);
    }
    /* pass 2 pull triples without host*/
    for(i=0;i<len;i++) {
        NCTriple* ti = nclistget(tmprc,i);
	if(ti->host != NULL) continue;
	nclistpush(rc,ti);
    }
#ifdef D4DEBUG
    storedump("reorder:",rc);
#endif
    nclistfree(tmprc);
}

/* Create a triple store from a file */
static int
rccompile(const char* path)
{
    int ret = NC_NOERR;
    NClist* rc = NULL;
    char* contents = NULL;
    NCbytes* tmp = ncbytesnew();
    NCURI* uri = NULL;
    char* nextline = NULL;
    NCRCglobalstate* globalstate = ncrc_getglobalstate();

    if((ret=NC_readfile(path,tmp))) {
        nclog(NCLOGERR, "Could not open configuration file: %s",path);
	goto done;
    }
    contents = ncbytesextract(tmp);
    if(contents == NULL) contents = strdup("");
    /* Either reuse or create new  */
    rc = globalstate->rcinfo.triples;
    if(rc != NULL)
        rcfreetriples(rc); /* clear out any old data */
    else {
        rc = nclistnew();
        globalstate->rcinfo.triples = rc;
    }
    nextline = contents;
    for(;;) {
	char* line;
	char* key;
        char* value;
	size_t llen;
        NCTriple* triple;

	line = rcreadline(&nextline);
	if(line == NULL) break; /* done */
        rctrim(line);  /* trim leading and trailing blanks */
        if(line[0] == '#') continue; /* comment */
	if((llen=strlen(line)) == 0) continue; /* empty line */
	triple = (NCTriple*)calloc(1,sizeof(NCTriple));
	if(triple == NULL) {ret = NC_ENOMEM; goto done;}
	if(line[0] == LTAG) {
	    char* url = ++line;
            char* rtag = strchr(line,RTAG);
            if(rtag == NULL) {
                nclog(NCLOGERR, "Malformed [url] in %s entry: %s",path,line);
                free(triple);
		continue;
            }
            line = rtag + 1;
            *rtag = '\0';
            /* compile the url and pull out the host */
            if(uri) ncurifree(uri);
            if(ncuriparse(url,&uri)) {
                nclog(NCLOGERR, "Malformed [url] in %s entry: %s",path,line);
                free(triple);
		continue;
            }
            ncbytesclear(tmp);
            ncbytescat(tmp,uri->host);
            if(uri->port != NULL) {
		ncbytesappend(tmp,':');
                ncbytescat(tmp,uri->port);
            }
            ncbytesnull(tmp);
            triple->host = ncbytesextract(tmp);
	    if(strlen(triple->host)==0)
		{free(triple->host); triple->host = NULL;}
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
	triple->key = strdup(key);
        triple->value = strdup(value);
        rctrim(triple->key);
        rctrim(triple->value);
#ifdef D4DEBUG
	fprintf(stderr,"rc: host=%s key=%s value=%s\n",
		(triple->host != NULL ? triple->host : "<null>"),
		triple->key,triple->valu);
#endif
	nclistpush(rc,triple);
	triple = NULL;
    }
    rcorder(rc);

done:
    if(contents) free(contents);
    ncurifree(uri);
    ncbytesfree(tmp);
    return (ret);
}

/**
 * (Internal) Locate a triple by property key and host+port (may be null or "").
 * If duplicate keys, first takes precedence.
 */
static struct NCTriple*
rclocate(const char* key, const char* hostport)
{
    int i,found;
    NCRCglobalstate* globalstate = ncrc_getglobalstate();
    NClist* rc = globalstate->rcinfo.triples;
    NCTriple* triple = NULL;

    if(globalstate->rcinfo.ignore)
	return NULL;

    if(key == NULL || rc == NULL) return NULL;
    if(hostport == NULL) hostport = "";

    for(found=0,i=0;i<nclistlength(rc);i++) {
      int t;
      size_t hplen;
      triple = (NCTriple*)nclistget(rc,i);

      hplen = (triple->host == NULL ? 0 : strlen(triple->host));

        if(strcmp(key,triple->key) != 0) continue; /* keys do not match */
        /* If the triple entry has no url, then use it
           (because we have checked all other cases)*/
        if(hplen == 0) {found=1;break;}
        /* do hostport match */
	t = 0;
	if(triple->host != NULL)
            t = strcmp(hostport,triple->host);
        if(t ==  0) {found=1; break;}
    }
    return (found?triple:NULL);
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

    size_t pathlen = plen+rclen+1; /*+1 for '/' */
    path = (char*)malloc(pathlen+1); /* +1 for nul*/
    if(path == NULL) {ret = NC_ENOMEM;	goto done;}
    strncpy(path,prefix,pathlen);
    strncat(path,"/",pathlen);
    strncat(path,rcname,pathlen);
    /* see if file is readable */
    f = fopen(path,"r");
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
    return (ret);
}

int
NC_rcfile_insert(const char* key, const char* value, const char* hostport)
{
    int ret = NC_NOERR;
    /* See if this key already defined */
    struct NCTriple* triple = NULL;
    NCRCglobalstate* globalstate = ncrc_getglobalstate();
    NClist* rc = globalstate->rcinfo.triples;

    if(rc == NULL) {
	rc = nclistnew();
	if(rc == NULL) {ret = NC_ENOMEM; goto done;}
    }
    triple = rclocate(key,hostport);
    if(triple == NULL) {
	triple = (NCTriple*)calloc(1,sizeof(NCTriple));
	if(triple == NULL) {ret = NC_ENOMEM; goto done;}
	triple->key = strdup(key);
	triple->value = NULL;
        rctrim(triple->key);
        triple->host = (hostport == NULL ? NULL : strdup(hostport));
	nclistpush(rc,triple);
    }
    if(triple->value != NULL) free(triple->value);
    triple->value = strdup(value);
    rctrim(triple->value);
done:
    return ret;
}

/* Obtain the count of number of triples */
size_t
NC_rcfile_length(NCRCinfo* info)
{
    return nclistlength(info->triples);
}

/* Obtain the ith triple; return NULL if out of range */
NCTriple*
NC_rcfile_ith(NCRCinfo* info, size_t i)
{
    if(i >= nclistlength(info->triples))
	return NULL;
    return (NCTriple*)nclistget(info->triples,i);
}


#ifdef D4DEBUG
static void
storedump(char* msg, NClist* triples)
{
    int i;

    if(msg != NULL) fprintf(stderr,"%s\n",msg);
    if(triples == NULL || nclistlength(triples)==0) {
        fprintf(stderr,"<EMPTY>\n");
        return;
    }
    for(i=0;i<nclistlength(triples);i++) {
	NCTriple* t = (NCTriple*)nclistget(triples,i);
        fprintf(stderr,"\t%s\t%s\t%s\n",
                ((t->host == NULL || strlen(t->host)==0)?"--":t->host),t->key,t->value);
    }
    fflush(stderr);
}
#endif
