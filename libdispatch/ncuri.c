/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "ncuri.h"
#include "ncbytes.h"
#include "nclist.h"

/* Include netcdf.h to allow access to
   NC_ error return codes. */
#include "netcdf.h"

#define NCURIDEBUG

/* Extra debug info */
#undef NCXDEBUG

#ifdef NCURIDEBUG
#define THROW(n) {ret=(n); goto done;}
#else
#define THROW(n) {goto done;}
#endif

#define PADDING 8

#define LBRACKET '['
#define RBRACKET ']'
#define EOFCHAR '\0'
#define RBRACKETSTR "]"

#define DRIVELETTERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

#ifndef FIX
#define FIX(s) ((s)==NULL?"NULL":(s))
#endif

#ifndef NILLEN
#define NILLEN(s) ((s)==NULL?0:strlen(s))
#endif

#ifndef nulldup
#define nulldup(s) ((s)==NULL?NULL:strdup(s))
#endif

#define terminate(p) {*(p) = EOFCHAR;}

#define endof(p) ((p)+strlen(p))

#define lshift(buf,buflen) {memmove(buf,buf+1,buflen+1);}
#define rshift(buf,buflen) {memmove(buf+1,buf,buflen+1);}

/* Allowable character sets for encode */
static const char* pathallow =
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$&'()*+,-./:;=?@_~";

static const char* queryallow =
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$&'()*+,-./:;=?@_~";

/* user+pwd allow = path allow - "@:" */
static const char* userpwdallow =
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!$&'()*+,-.;=_~?#/";

#ifndef HAVE_STRNDUP
#define strndup ncstrndup
/* Not all systems have strndup, so provide one*/
char*
ncstrndup(const char* s, size_t len)
{
    char* dup;
    if(s == NULL) return NULL;
    dup = (char*)malloc(len+1);
    if(dup == NULL) return NULL;
    memcpy((void*)dup,s,len);
    dup[len] = '\0';
    return dup;
}
#endif
/* Forward */
static int collectprefixparams(char* text, char** nextp);
static void freestringlist(NClist* list);
static void freestringvec(char** list);
static int ncfind(char** params, const char* key);
static char* nclocate(char* p, const char* charlist);
static int parselist(const char* ptext, NClist* list);

/**************************************************/
/*
A note about parameter support:
In the original url format for opendap (dap2), client parameters were
assumed to be one or more instances of bracketed pairs: e.g
    "[...][...]...".
These were assumed to be placed at the front of the url.  In this newer
version, the parameters may be encoded after a trailing # character each
separated by ampersand (&).  For back compatibility, the bracketed
parameter form is supported. However, if ncuribuild is used, all
parameters will be converted to the
    #...&...& format.
In any case, each parameter in turn is assumed to be a of the form
<name>=<value> or <name>; e.g. #x=y&z&a=b&w.
If the same parameter is specified more than once, then the first
occurrence is used; this is so that is possible to forcibly override
user specified parameters by prefixing.
IMPORTANT: the client parameter string is assumed to have blanks compressed out.
*/

/**************************************************/

/* Do a simple uri parse: return NC_NOERR if success, NC_EXXX if failed */
int
ncuriparse(const char* uri0, NCURI** durip)
{
    int ret = NC_NOERR;
    NCURI tmp;
    char* p;
    char* q;
    int isfile;
    int hashost;
    char* uri = NULL;
    NCURI* duri = NULL;
    char* prefix = NULL;
    char* next = NULL;
    NClist* params = nclistnew();
    NClist* querylist = nclistnew();
    size_t len0;
    int pathchar;

    tmp.fraglist = NULL;
    tmp.querylist = NULL;

    if(uri0 == NULL)
	{THROW(NC_EURL);}

    len0 = strlen(uri0);
    if(len0 == 0)
	{THROW(NC_EURL);}

    /* Create a local NCURI instance to hold
       pointers into the parsed string
    */
    memset(&tmp,0,sizeof(tmp));

    /* make mutable copy. Add some extra space
       because we will need to null terminate the host section
       without losing the first character of the path section.
    */
    uri = (char*)malloc(len0+1+1); /* +2 for nul term and for host section terminator */
    if(uri == NULL)
	{THROW(NC_ENOMEM);}
    strncpy(uri,uri0,len0+1);

    /* Walk the uri and do the following:
	1. remove leading and trailing whitespace
	2. convert all '\\' -> '\' (Temp hack to remove escape characters
                                    inserted by Windows or MinGW)
    */
    for(q=uri,p=uri;*p;p++) {if((*p == '\\' && p[1] == '\\') || *p < ' ') {continue;} else {*q++ = *p;}}
    *q = '\0';

    p = uri;

    /* break up the url into coarse pieces */
    if(*p == LBRACKET) {
        prefix = p;
        ret = collectprefixparams(p,&next); /* collect the prefix */
        if(ret != NC_NOERR)
            {THROW(NC_EURL);}
         p = next;
    } else {
	prefix = NULL;
    }
    tmp.uri = p; /* will be the core */
    /* Skip past the core of the url */
    next = nclocate(p,"?#");
    if(next != NULL) {
	int c = *next;
	terminate(next);
	next++;
	if(c == '?') {
	    tmp.query = next;
	    next = nclocate(next,"#");
	    if(next == NULL)
		tmp.fragment = NULL;
	    else {
		terminate(next);
		next++;
	        tmp.fragment = next;
	    }
	} else { /*c == '#'*/
	    tmp.fragment = next;
	}
    }

    /* Parse the prefix parameters */
    if(prefix != NULL) {
        if(parselist(prefix,params) != NC_NOERR)
            {THROW(NC_EURL);}
    }
    /* Parse the fragment parameters */
    if(tmp.fragment != NULL) {
        if(parselist(tmp.fragment,params) != NC_NOERR)
            {THROW(NC_EURL);}
    }
    if(nclistlength(params) > 0) {
	nclistpush(params,NULL);
        tmp.fraglist = nclistextract(params);
    } else
	tmp.fraglist = NULL;
    /* Parse the query */
    if(tmp.query != NULL) {
        if(parselist(tmp.query,querylist) != NC_NOERR)
            {THROW(NC_EURL);}
        if(nclistlength(querylist) > 0) {
	    nclistpush(querylist,NULL);
            tmp.querylist = nclistextract(querylist);
	}
    }

    /* Now parse the core of the url */
    p = tmp.uri;

    /* Mark the protocol */
    tmp.protocol = p;
    p = strchr(p,':');
    if(!p)
	{THROW(NC_EURL);}
    terminate(p); /*overwrite colon*/
    p++; /* skip the colon */
    if(strlen(tmp.protocol)==0)
	{THROW(NC_EURL);}
    /*
       The legal formats for file: urls are a problem since
       many variants are often accepted.
       By RFC, the proper general format is: file://host/path,
       where the 'host' can be omitted and defaults to 'localhost'.
       and the path includes the leading '/'.
       So, assuming no host, the format is: "file:///path".
       Some implementations, however, ignore the host, and allow
       the format: file:/path.
       We also simplify things by assuming the host part is always empty.
       which means we can have file:///path, but not file://..../path.
       Note also in all cases, the leading '/' is considered part of the path,
       which is then assumed to be an absolute path. But also note that
       the windows drive letter has to be taken into account. Our rule is that
       if the path looks like D:...,
       where D is a single alphabetic letter (a-z or A-Z),
       then it is a windows path and can be use in place of a /path.
       Note also that it is desirable to support relative paths even
       though the RFC technically does not allow this. This will occur
       if the form is file://path where path does not start with '/'.
       The rules implemented here (for file:) are then as follows
       1. file:D:... : assume D: is a windows drive letter and treat D:... as the path
       2. file:/X, where X does not start with a slash: treat /X as the path.
       3. file://D:... : assume D: is a windows drive letter and treat as the path
       4. file:///X, where X does not start with a slash: treat /X as the path.
       5. file://X, where X does not start with a slash: treat X as the
          relative path.
       All other cases are disallowed.
    */

    isfile = (strcmp(tmp.protocol,"file")==0);
    if(isfile) {
	size_t l = strlen(p); /* to test if we have enough characters */
	hashost = 0; /* always */
	if(l >= 2 && p[1] == ':' && strchr(DRIVELETTERS,p[0]) != NULL) { /* case 1 */
	    ; /* p points to the start of the path */
        } else if(l >= 2 && p[0] == '/' && p[1] != '/') { /* case 2 */
	    ; /* p points to the start of the path */
	} else if(l >= 4 && p[0] == '/' && p[1] == '/'
		&& p[3] == ':' && strchr(DRIVELETTERS,p[2]) != NULL) { /* case 3 */
	    p = p+2; /* points to the start of the windows path */
        } else if(l >= 4 && p[0] == '/' && p[1] == '/' && p[2] == '/' && p[3] != '/') { /* case 4 */
	    p += 2; /* points to the start of the path */
        } else if(l >= 4 && p[0] == '/' && p[1] == '/' && p[2] != '/') { /* case 5 */
	    p += 2; /* points to the start of the path */
        } else /* everything else is illegal */
	    {THROW(NC_EACCESS);}
    } else {
        if(p[0] != '/' || p[1] != '/') /* must be proto:// */
	    {THROW(NC_EACCESS);}
	p += 2;
        hashost = 1; /* Assume we have a hostname */
    }
    if(!hashost) {
        tmp.path = p;
	pathchar = EOFCHAR;
    } else { /* assume there should be a host section */
	/* We already extracted the query and/or fragment sections above,
           splocate the end of the host section and therefore the start
           of the path.
        */
	tmp.host = p;
        p  = nclocate(p,"/");
	if(p == NULL) { /* no path */
	    tmp.path = NULL; /* default */
	    pathchar = EOFCHAR;
	} else {
	    tmp.path = p; /* save ptr to rest of the path */
	    pathchar = *p; /* save leading char of the path */
	    terminate(p); /* overwrite the leading char of the path; restored below */
	}
    }
    /* Nullify tmp.host for consistency */
    if(tmp.host != NULL && strlen(tmp.host)==0) {tmp.host = NULL;}

    if(tmp.host != NULL) {/* Parse the host section */
        char* pp;
	/* Check for leading user:pwd@ */
        char* newhost = strchr(tmp.host,'@');
        if(newhost != NULL) {
	    if(newhost == tmp.host)
		{THROW(NC_EURL);} /* we have proto://@ */
	    terminate(newhost); /* overwrite '@' */
	    newhost++; /* should point past usr+pwd */
	    tmp.user = tmp.host;
	    /* Break user+pwd into two pieces */
	    pp = strchr(tmp.user,':');
	    if(pp == NULL)
		{THROW(NC_EURL);} /* we have user only */
	    terminate(pp); /* overwrite ':' */
	    pp++;
	    if(strlen(tmp.user)==0)
		{THROW(NC_EURL);} /* we have empty user */
	    if(strlen(pp)==0)
		{THROW(NC_EURL);} /* we have empty password */
	    tmp.password = pp;
	    tmp.host = newhost;
	}
	/* Breakup host into host + port */
	pp = tmp.host;
        pp = strchr(pp,':');
        if(pp != NULL) { /* there is a port */
	    terminate(pp); /* overwrite ':' */
	    pp++; /* skip colon */
	    if(strlen(tmp.host) == 0)
		{THROW(NC_EURL);} /* empty host */
	    if(strlen(pp)==0)
		{THROW(NC_EURL);} /* empty port */
	    tmp.port = pp;
	    /* The port must look something like a number */
	    for(pp=tmp.port;*pp;pp++) {
	        if(strchr("0123456789-",*pp) == NULL)
		    {THROW(NC_EURL);}  /* probably not a real port, fail */
	    }
	} /* else no port */
    }

    /* Fill in duri from tmp */
    duri = (NCURI*)calloc(1,sizeof(NCURI));
    if(duri == NULL)
      {THROW(NC_ENOMEM);}
    /* save original uri */
    duri->uri = strdup(uri0);
    duri->protocol = nulldup(tmp.protocol);
    /* before saving, we need to decode the user+pwd */
    duri->user = NULL;
    duri->password = NULL;
    if(tmp.user != NULL)
        duri->user = ncuridecode(tmp.user);
    if(tmp.password != NULL)
        duri->password = ncuridecode(tmp.password);
    duri->host = nulldup(tmp.host);
    duri->port = nulldup(tmp.port);
    if(tmp.path != NULL) {
	/* We need to add back the previously overwritten path lead char (if necessary);
           this must be done after all host section related pieces have been captured */
	if(pathchar != EOFCHAR)
	    *tmp.path = pathchar;
        duri->path = nulldup(tmp.path);
    }
    duri->query = nulldup(tmp.query);
    duri->fragment = nulldup(tmp.fragment);
    duri->fraglist = tmp.fraglist; tmp.fraglist = NULL;
    duri->querylist = tmp.querylist; tmp.querylist = NULL;
    if(durip)
      *durip = duri;
    else
      free(duri);

#ifdef NCXDEBUG
	{
        fprintf(stderr,"duri:");
	fprintf(stderr," protocol=|%s|",FIX(duri->protocol));
	fprintf(stderr," user=|%s|",FIX(duri->user));
	fprintf(stderr," password=|%s|",FIX(duri->password));
	fprintf(stderr," host=|%s|",FIX(duri->host));
	fprintf(stderr," port=|%s|",FIX(duri->port));
	fprintf(stderr," path=|%s|",FIX(duri->path));
	fprintf(stderr," query=|%s|",FIX(duri->query));
	fprintf(stderr," fragment=|%s|",FIX(duri->fragment));
        fprintf(stderr,"\n");
    }
#endif

done:
    if(uri != NULL)
      free(uri);

    freestringlist(params);
    freestringlist(querylist);
    if(tmp.fraglist)
      freestringvec(tmp.fraglist);
    if(tmp.querylist)
      freestringvec(tmp.querylist);

    return ret;
}

static void
freestringlist(NClist* list)
{
    if(list != NULL) {
	int i;
	for(i=0;i<nclistlength(list);i++) {
	    void* p = nclistget(list,i);
	    nullfree(p);
	}
        nclistfree(list);
    }
}

static void
freestringvec(char** list)
{
    if(list != NULL) {
	char** p;
        for(p=list;*p;p++) {nullfree(*p);}
	nullfree(list);
    }
}

void
ncurifree(NCURI* duri)
{
    if(duri == NULL) return;
    nullfree(duri->uri);
    nullfree(duri->protocol);
    nullfree(duri->user);
    nullfree(duri->password);
    nullfree(duri->host);
    nullfree(duri->port);
    nullfree(duri->path);
    nullfree(duri->query);
    nullfree(duri->fragment);
    freestringvec(duri->querylist);
    freestringvec(duri->fraglist);
    free(duri);
}

/* Replace the protocol */
int
ncurisetprotocol(NCURI* duri,const char* protocol)
{
    nullfree(duri->protocol);
    duri->protocol = strdup(protocol);
    return (NC_NOERR);
}

/* Replace the query */
int
ncurisetquery(NCURI* duri,const char* query)
{
    int ret = NC_NOERR;
    freestringvec(duri->querylist);
    nullfree(duri->query);
    duri->query = NULL;
    duri->querylist = NULL;
    if(query != NULL && strlen(query) > 0) {
	NClist* params = nclistnew();
	duri->query = strdup(query);
	ret = parselist(duri->query,params);
	if(ret != NC_NOERR)
	    {THROW(NC_EURL);}
	nclistpush(params,NULL);
	duri->querylist = nclistextract(params);
	nclistfree(params);
    }
done:
    return ret;
}

/* Replace the fragments*/
int
ncurisetfragments(NCURI* duri,const char* fragments)
{
    int ret = NC_NOERR;
    freestringvec(duri->fraglist);
    nullfree(duri->fragment);
    duri->fragment = NULL;
    duri->fraglist = NULL;
    if(fragments != NULL && strlen(fragments) > 0) {
	NClist* params = nclistnew();
	duri->fragment = strdup(fragments);
	ret = parselist(duri->fragment,params);
	if(ret != NC_NOERR)
	    {THROW(NC_EURL);}
	nclistpush(params,NULL);
	duri->fraglist = nclistextract(params);
	nclistfree(params);
    }
done:
    return ret;
}

#if 0
/* Replace the constraints */
int
ncurisetconstraints(NCURI* duri,const char* constraints)
{
    char* proj = NULL;
    char* select = NULL;
    const char* p;

    if(duri->constraint != NULL) free(duri->constraint);
    if(duri->projection != NULL) free(duri->projection);
    if(duri->selection != NULL) free(duri->selection);
    duri->constraint = NULL;
    duri->projection = NULL;
    duri->selection = NULL;

    if(constraints == NULL || strlen(constraints)==0) return (NC_ECONSTRAINTS);

    duri->constraint = nulldup(constraints);
    if(*duri->constraint == '?')
	nclshift1(duri->constraint);

    p = duri->constraint;
    proj = (char*) p;
    select = strchr(proj,'&');
    if(select != NULL) {
        size_t plen = (size_t)(select - proj);
	if(plen == 0) {
	    proj = NULL;
	} else {
	    proj = (char*)malloc(plen+1);
	    memcpy((void*)proj,p,plen);
	    proj[plen] = EOFCHAR;
	}
	select = nulldup(select);
    } else {
	proj = nulldup(proj);
	select = NULL;
    }
    duri->projection = proj;
    duri->selection = select;
    return NC_NOERR;
}
#endif

/* Construct a complete NC URI.
   Optionally with the constraints.
   Optionally with the user parameters.
   Caller frees returned string.
   Optionally encode the pieces.
*/

char*
ncuribuild(NCURI* duri, const char* prefix, const char* suffix, int flags)
{
    char* newuri = NULL;
    NCbytes* buf = ncbytesnew();
    const int encode = (flags&NCURIENCODE ? 1 : 0);

    if(prefix != NULL)
	ncbytescat(buf,prefix);

    ncbytescat(buf,duri->protocol);
    ncbytescat(buf,"://"); /* this will produce file:///... */

    if((flags & NCURIPWD) && duri->user != NULL && duri->password != NULL) {
	/* The user and password must be encoded */
        char* encoded = ncuriencodeonly(duri->user,userpwdallow);
	ncbytescat(buf,encoded);
	nullfree(encoded);
	ncbytescat(buf,":");
	encoded = ncuriencodeonly(duri->password,userpwdallow);
	ncbytescat(buf,encoded);
	nullfree(encoded);
	ncbytescat(buf,"@");
    }
    if(duri->host != NULL) ncbytescat(buf,duri->host);
    if(duri->port != NULL) {
	ncbytescat(buf,":");
	ncbytescat(buf,duri->port);
    }
    if((flags & NCURIPATH)) {
	if(duri->path == NULL)
	    ncbytescat(buf,"/");
	else if(encode) {
	    char* encoded = ncuriencodeonly(duri->path,pathallow);
	    ncbytescat(buf,encoded);
	    nullfree(encoded);
	} else
	    ncbytescat(buf,duri->path);
    }

    /* The suffix is intended to some kind of path extension (e.g. .dds)
       so insert here
    */
    if(suffix != NULL)
	ncbytescat(buf,suffix);

    /* The query and the querylist are assumed to be unencoded */
    if(flags & NCURIQUERY && duri->querylist != NULL) {
	char** p;
	int first = 1;
	for(p=duri->querylist;*p;p+=2,first=0) {
	    ncbytescat(buf,(first?"?":"&"));
	    if(encode) {
		char* encoded = ncuriencodeonly(p[0],queryallow);
		ncbytescat(buf,encoded);
	        nullfree(encoded);
	    } else
	        ncbytescat(buf,p[0]);
	    if(p[1] != NULL && strlen(p[1]) > 0) {
		ncbytescat(buf,"=");
		if(encode) {
		    char* encoded = ncuriencodeonly(p[1],queryallow);
		    ncbytescat(buf,encoded);
	            nullfree(encoded);
		} else
		    ncbytescat(buf,p[1]);
	    }
	}
    }
    if((flags & NCURIFRAG) && duri->fraglist != NULL) {
	char** p;
	int first = 1;
	for(p=duri->fraglist;*p;p+=2,first=0) {
	    ncbytescat(buf,(first?"#":"&"));
	    ncbytescat(buf,p[0]);
	    if(p[1] != NULL && strlen(p[1]) > 0) {
		ncbytescat(buf,"=");
		if(encode) {
		    char* encoded = ncuriencodeonly(p[1],queryallow);
		    ncbytescat(buf,encoded);
	            nullfree(encoded);
		} else
		    ncbytescat(buf,p[1]);
	    }
	}
    }
    ncbytesnull(buf);
    newuri = ncbytesextract(buf);
    ncbytesfree(buf);
    return newuri;
}


const char*
ncurilookup(NCURI* uri, const char* key)
{
  int i;
  char* value = NULL;
  if(uri == NULL || key == NULL || uri->fraglist == NULL) return NULL;
  i = ncfind(uri->fraglist,key);
  if(i < 0)
    return NULL;
  value = uri->fraglist[(2*i)+1];
  return value;
}

const char*
ncuriquerylookup(NCURI* uri, const char* key)
{
  int i;
  char* value = NULL;
  if(uri == NULL || key == NULL || uri->querylist == NULL) return NULL;
  i = ncfind(uri->querylist,key);
  if(i < 0)
    return NULL;
  value = uri->querylist[(2*i)+1];
  return value;
}

/* Obtain the complete list of fragment pairs in envv format */
const char**
ncurifragmentparams(NCURI* uri)
{
    return (const char**)uri->fraglist;
}

/* Obtain the complete list of query pairs in envv format */
const char**
ncuriqueryparams(NCURI* uri)
{
    return (const char**)uri->querylist;
}

#if 0
int
ncuriremoveparam(NCURI* uri, const char* key)
{
    char** p;
    char** q = NULL;

    if(uri->fraglist == NULL) return NC_NOERR;
    for(q=uri->fraglist,p=uri->fraglist;*p;) {
        if(strcmp(key,*p)==0) {
	    p += 2; /* skip this entry */
	} else {
	    *q++ = *p++; /* move key */
	    *q++ = *p++; /* move value */
	}
    }
    return NC_NOERR;
}
#endif


/* Internal version of lookup; returns the paired index of the key;
   case insensitive
 */
static int
ncfind(char** params, const char* key)
{
    int i;
    char** p;
    for(i=0,p=params;*p;p+=2,i++) {
	if(strcasecmp(key,*p)==0) return i;
    }
    return -1;
}


#if 0
static void
ncparamfree(char** params)
{
    char** p;
    if(params == NULL) return;
    for(p=params;*p;p+=2) {
	free(*p);
	if(p[1] != NULL) free(p[1]);
    }
    free(params);
}
#endif

/* Return the ptr to the first occurrence of
   any char in the list. Return NULL if no
   occurrences
*/
static char*
nclocate(char* p, const char* charlist)
{
    for(;*p;p++) {
	if(*p == '\\') p++;
	else if(strchr(charlist,*p) != NULL)
	    return p;
    }
    return NULL;
}

#if 0
/* Shift every char starting at p 1 place to the left */
static void
nclshift1(char* p)
{
    if(p != NULL && *p != EOFCHAR) {
	char* q = p++;
	while((*q++=*p++));
    }
}

/* Shift every char starting at p 1 place to the right */
static void
ncrshift1(char* p)
{
    char cur;
    cur = 0;
    do {
	char next = *p;
	*p++ = cur;
	cur = next;
    } while(cur != 0);
    *p = 0; /* make sure we are still null terminated */
}
#endif

/* Provide % encoders and decoders */

static const char* hexchars = "0123456789abcdefABCDEF";

static void
toHex(unsigned int b, char hex[2])
{
    hex[0] = hexchars[(b >> 4) & 0xf];
    hex[1] = hexchars[(b) & 0xf];
}


static int
fromHex(int c)
{
    if(c >= '0' && c <= '9') return (int) (c - '0');
    if(c >= 'a' && c <= 'f') return (int) (10 + (c - 'a'));
    if(c >= 'A' && c <= 'F') return (int) (10 + (c - 'A'));
    return 0;
}

/*
Support encode of user and password fields
*/
char*
ncuriencodeuserpwd(char* s)
{
    return ncuriencodeonly(s,userpwdallow);
}

/* Return a string representing encoding of input; caller must free;
   watch out: will encode whole string, so watch what you give it.
   Allowable argument specifies characters that do not need escaping.
 */

char*
ncuriencodeonly(char* s, const char* allowable)
{
    size_t slen;
    char* encoded;
    char* inptr;
    char* outptr;

    if(s == NULL) return NULL;

    slen = strlen(s);
    encoded = (char*)malloc((3*slen) + 1); /* max possible size */

    for(inptr=s,outptr=encoded;*inptr;) {
	int c = *inptr++;
        if(c == ' ') {
	    *outptr++ = '+';
        } else {
            /* search allowable */
	    char* p = strchr(allowable,c);
	    if(p != NULL) {
                *outptr++ = (char)c;
            } else {
		char hex[2];
		toHex(c,hex);
		*outptr++ = '%';
		*outptr++ = hex[0];
		*outptr++ = hex[1];
            }
        }
    }
    *outptr = EOFCHAR;
    return encoded;
}

/* Return a string representing decoding of input; caller must free;*/
char*
ncuridecode(char* s)
{
    size_t slen;
    char* decoded;
    char* outptr;
    char* inptr;
    unsigned int c;

    if (s == NULL) return NULL;

    slen = strlen(s);
    decoded = (char*)malloc(slen+1); /* Should be max we need */

    outptr = decoded;
    inptr = s;
    while((c = (unsigned int)*inptr++)) {
	if(c == '%') {
            /* try to pull two hex more characters */
	    if(inptr[0] != EOFCHAR && inptr[1] != EOFCHAR
		&& strchr(hexchars,inptr[0]) != NULL
		&& strchr(hexchars,inptr[1]) != NULL) {
		/* test conversion */
		int xc = (fromHex(inptr[0]) << 4) | (fromHex(inptr[1]));
		inptr += 2; /* decode it */
		c = (unsigned int)xc;
            }
        }
        *outptr++ = (char)c;
    }
    *outptr = EOFCHAR;
    return decoded;
}

/*
Partially decode a string. Only characters in 'decodeset'
are decoded. Return decoded string; caller must free.
*/
char*
ncuridecodepartial(char* s, const char* decodeset)
{
    size_t slen;
    char* decoded;
    char* outptr;
    char* inptr;
    unsigned int c;

    if (s == NULL || decodeset == NULL) return NULL;

    slen = strlen(s);
    decoded = (char*)malloc(slen+1); /* Should be max we need */

    outptr = decoded;
    inptr = s;
    while((c = (unsigned int)*inptr++)) {
	if(c == '+' && strchr(decodeset,'+') != NULL)
	    *outptr++ = ' ';
	else if(c == '%') {
            /* try to pull two hex more characters */
	    if(inptr[0] != EOFCHAR && inptr[1] != EOFCHAR
		&& strchr(hexchars,inptr[0]) != NULL
		&& strchr(hexchars,inptr[1]) != NULL) {
		/* test conversion */
		int xc = (fromHex(inptr[0]) << 4) | (fromHex(inptr[1]));
		if(strchr(decodeset,xc) != NULL) {
		    inptr += 2; /* decode it */
		    c = (unsigned int)xc;
		}
            }
            *outptr++ = (char)c; /* pass either the % or decoded char */
        } else /* Not a % char */
            *outptr++ = (char)c;
    }
    *outptr = EOFCHAR;
    return decoded;
}

static int
collectprefixparams(char* text, char** nextp)
{
    int ret = NC_NOERR;
    char* sp;
    char* ep;
    char* last;

    if(text == NULL) return NC_EURL;
    if(strlen(text) == 0) {
	if(nextp) *nextp = text;
	return NC_NOERR;
    }
    /* pass 1: locate last rbracket and nul term the prefix */
    sp = text;
    last = NULL;
    for(;;) {
	if(*sp != LBRACKET) {
	    if(nextp) *nextp = sp;
	    break;
	}
        /* use nclocate because \\ escapes might be present */
        ep = nclocate(sp,RBRACKETSTR);
	if(ep == NULL) {ret = NC_EINVAL; goto done;} /* malformed */
	last = ep; /* save this position  */
	ep++; /* move past rbracket */
	sp = ep;
    }
    /* nul terminate */
    if(last != NULL)
	terminate(last);

    /* pass 2: convert [] to & */
    sp = text;
    for(;;) {
	char* p; char* q;
	/* by construction, here we are at an LBRACKET: compress it out */
	for(p=sp,q=sp+1;(*p++=*q++);)
	    ;
        /* locate the next RRACKET */
        ep = nclocate(sp,RBRACKETSTR);
	if(ep == NULL) break;/* we are done */
	/* convert the BRACKET to '&' */
	*ep = '&';
	ep++; /* move past rbracket */
	sp = ep;
    }
done:
    return ret;
}

static int
parselist(const char* text, NClist* list)
{
    int ret = NC_NOERR;
    char* ptext = NULL;
    char* p;
    ptext = strdup(text); /* We need to modify */
    p = ptext; /* start of next parameter */
    for(;;) {
	char* sp = p;
	char* eq;
	char* ep;
	char* key;
	char* value;
	if(*p == EOFCHAR) break; /* we are done */
        /* use nclocate because \\ escapes might be present */
	ep = nclocate(sp,"&");
	if(ep != NULL) {
	    terminate(ep); /* overwrite the trailing ampersand */
	    p = ep+1; /* next param */
	}
	/* split into key + value */
        eq = strchr(sp,'=');
        if(eq != NULL) { /* value is present */
	    terminate(eq); eq++;
	    key = strdup(sp);
	    value = strdup(eq);
	} else {/* there is no value */
	    key = strdup(sp);
	    value = strdup("");
	}
        nclistpush(list,key);
	nclistpush(list,value);
	if(ep == NULL)
	    break;
    }
    nullfree(ptext);
    return ret;
}
