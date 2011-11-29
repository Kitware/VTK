/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

#include "ncdispatch.h"
#include "nc_url.h"

#define LBRACKET '['
#define RBRACKET ']'

static NClist* nc_urlparamdecode(char* params0);
static NClist* nc_urlparamlookup(NClist* params, const char* clientparam);
static void nc_urlparamfree(NClist* params);

/* Do a simple url parse*/
int
nc_urlparse(const char* url0, NC_URL** ncurlp)
{
    NCerror ncstat = NC_NOERR;
    char* url = NULL;
    char* p;
    char* p1;
    int c;
    NC_URL* ncurl;
    size_t protolen;

    /* accumulate parse points*/
    char* protocol = NULL;
    char* params = NULL;
    char* baseurl = NULL;
    char* constraint = NULL;
    char* stop;

    /* copy url and remove all whitespace*/
    url = strdup(url0);
    if(url == NULL) return NC_ENOMEM;

    p = url;
    p1 = url;
    while((c=*p1++)) {if(c != ' ' && c != '\t') *p++ = c;}
    *p = '\0';

    p = url;
    stop = p + strlen(p);

    /* break up the url string into pieces*/
    if(*p == LBRACKET) {
    params = p+1;
/* find end of the clientparams*/
    for(;*p;p++) {if(p[0] == RBRACKET && p[1] != LBRACKET) break;}
    if(*p == 0) {
    ncstat = NC_EINVAL; /* malformed client params*/
    goto done;
    }
    *p = '\0'; /* leave off the trailing rbracket for now */
    p++; /* move past the params*/
    }

    baseurl = p;

    /* Note that we dont care what the protocol is ; just collect it */
    /* find the end of the protocol */
    p1 = strchr(p,':');
    if(p1 == NULL || p1 == p) {
    ncstat = NC_EINVAL; /* missing protocol*/
    goto done;
    }
    /* Check that the : is followed by "//" */
    if(p1[1] != '/' || p1[2] != '/') {
    ncstat = NC_EINVAL;
    goto done;
    }

    /* Simulate strndup */
    protolen = (size_t)(p1-p);
    protocol = malloc(1+protolen);
    if(protocol == NULL) return NC_ENOMEM;
    strncpy(protocol,p,protolen);
    protocol[protolen] = '\0';
    /* Look for '?' */
    constraint = strchr(p,'?');
    if(constraint) {
    *constraint++ = '\0';
    }

    /* assemble the component pieces*/
    ncurl = malloc(sizeof(NC_URL));
    if(ncurl == NULL) return NC_ENOMEM;
    memset((void*)ncurl,0,sizeof(NC_URL));

    ncurl->url = nulldup(url0);
    if(ncurl->url == NULL) return NC_ENOMEM;
    ncurl->base = nulldup(baseurl);
    if(ncurl->base == NULL) return NC_ENOMEM;
    ncurl->protocol = protocol;
    ncurl->constraint = nulldup(constraint);
    if(constraint != NULL && ncurl->constraint == NULL) return NC_ENOMEM;
    nc_urlsetconstraints(ncurl,constraint);
    if(params != NULL) {
    ncurl->params = (char*)malloc(1+2+strlen(params));
    if(ncurl->params == NULL) return NC_ENOMEM;
    strcpy(ncurl->params,"[");
    strcat(ncurl->params,params);
    strcat(ncurl->params,"]");
    }
    if(ncurlp) *ncurlp = ncurl;

#ifdef DEBUG
    fprintf(stderr,"urlparse: params=|%s| base=|%s| projection=|%s| selection=|%s|\n",
            ncurl->params, ncurl->base, ncurl->projection, ncurl->selection);
#endif

done:
    if(url != NULL) free(url);
    return ncstat;

}

/* Call must free the actual url instance.*/
void
nc_urlfree(NC_URL* ncurl)
{
    if(ncurl == NULL) return;
    if(ncurl->url != NULL) {free(ncurl->url);}
    if(ncurl->base != NULL) {free(ncurl->base);}
    if(ncurl->protocol != NULL) {free(ncurl->protocol);}
    if(ncurl->constraint != NULL) {free(ncurl->constraint);}
    if(ncurl->projection != NULL) {free(ncurl->projection);}
    if(ncurl->selection != NULL) {free(ncurl->selection);}
    if(ncurl->params != NULL) {free(ncurl->params);}
    if(ncurl->parammap != NULL) nc_urlparamfree(ncurl->parammap);
    free(ncurl);
}

/* Replace the constraints */
void
nc_urlsetconstraints(NC_URL* durl,const char* constraints)
{
    char* proj = NULL;
    char* select = NULL;
    const char* p;

    if(durl->projection != NULL) free(durl->projection);
    if(durl->selection != NULL) free(durl->selection);
    durl->projection = NULL;
    durl->selection = NULL;

    if(constraints == NULL || strlen(constraints)==0) return;

    p = constraints;
    if(p[0] == '?') p++;
    proj = (char*) p;
    select = strchr(proj,'&');
    if(select != NULL) {
    size_t plen = (select - proj);
    if(plen == 0) {
    proj = NULL;
    } else {
    proj = (char*)malloc(plen+1);
    if(proj == NULL) return;
    memcpy((void*)proj,p,plen);
    proj[plen] = '\0';
    }
    select = nulldup(select);
    } else {
    proj = nulldup(proj);
    select = NULL;
    }
    durl->projection = proj;
    durl->selection = select;
}

int
nc_urldecodeparams(NC_URL* ncurl)
{
    int ok = 0;
    if(ncurl->parammap == NULL && ncurl->params != NULL) {
    NClist* map = nc_urlparamdecode(ncurl->params);
    ncurl->parammap = map;
    ok = 1;
    }
    return ok;
}

/*! NULL result => entry not found.
    Empty value should be represented as a zero length list */
NClist*
nc_urllookup(NC_URL* durl, const char* clientparam)
{
    /* make sure that durl->parammap exists */
    if(durl->parammap == NULL) nc_urldecodeparams(durl);
    return nc_urlparamlookup(durl->parammap,clientparam);
}

/* Convenience: search a list for a given string; NULL if not found */
const char*
nc_urllookupvalue(NClist* list, const char* value)
{
    int i;
    if(list == NULL || value == NULL) return NULL;
    for(i=0;i<nclistlength(list);i++) {
    char* s = (char*)nclistget(list,i);
    if(s == NULL) continue;
    if(strcmp(value,s) == 0) return s;
    }
    return NULL;
}


/**************************************************/
/*

Client parameters are assumed to be one or more instances of
bracketed pairs: e.g "[...][...]...".  The bracket content
in turn is assumed to be a comma separated list of
<name>=<value> pairs.  e.g. x=y,z=,a=b.
The resulting parse is stored in a list where the
ith element is the name of the parameter
and the i+1'th element is a list
of all the occurrences kept in the original order.
*/

static NClist*
nc_urlparamdecode(char* params0)
{
    char* cp;
    char* cq;
    int c;
    int i;
    int nparams;
    NClist* map = nclistnew();
    char* params;
    char* tmp;

    if(params0 == NULL) return map;

    /* Pass 1 is to remove all blanks */
    params = strdup(params0);
    cp=params; cq = cp;
    while((c=*cp++)) {
    if(c == ' ') cp++; else *cq++ = c;
    }
    *cq = '\0';

    /* Pass 2 to replace beginning '[' and ending ']' */
    if(params[0] == '[')
      strcpy(params,params+1);
    if(params[strlen(params)-1] == ']')
      params[strlen(params)-1] = '\0';

    /* Pass 3 to replace "][" pairs with ','*/
    cp=params; cq = cp;;
    while((c=*cp++)) {
    if(c == RBRACKET && *cp == LBRACKET) {cp++; c = ',';}
    *cq++ = c;
    }
    *cq = '\0';

    /* Pass 4 to break string into pieces and count # of pairs */
    nparams=0;
    for(cp=params;(c=*cp);cp++) {
    if(c == ',') {*cp = '\0'; nparams++;}
    }
    nparams++; /* for last one */

    /* Pass 5 to break up each pass into a (name,value) pair*/
    /* and insert into the param map */
    /* parameters of the form name name= are converted to name=""*/
    cp = params;
    for(i=0;i<nparams;i++) {
    int j;
    char* next = cp+strlen(cp)+1; /* save ptr to next pair*/
    char* vp;
    NClist* values;
/*break up the ith param*/
    vp = strchr(cp,'=');
    if(vp != NULL) {*vp = '\0'; vp++;} else {vp = "";}
/* Locate any previous name match and get/create the value list*/
    for(values=NULL,j=0;j<nclistlength(map);j+=2) {
    if(strcmp(cp,(char*)nclistget(map,j))==0) {
    values = (NClist*)nclistget(map,j+1);
    break;
    }
    }
    if(values == NULL) {
    /*add at end */
    values = nclistnew();
    nclistpush(map,(ncelem)nulldup(cp));
    nclistpush(map,(ncelem)values);
    }
/* Add the value (may result in duplicates */
    nclistpush(values,(ncelem)nulldup(vp));
    cp = next;
    }
    free(params);
    return map;
}

/*
Lookup the param, if value is non-null, then see
if it occurs in the value list
*/

static NClist*
nc_urlparamlookup(NClist* params, const char* pname)
{
    int i;
    if(params == NULL || pname == NULL) return NULL;
    for(i=0;i<nclistlength(params);i+=2) {
    char* name = (char*)nclistget(params,i);
    if(strcmp(pname,name)==0) {
    return (NClist*)nclistget(params,i+1);
    }
    }
    return NULL;
}



static void
nc_urlparamfree(NClist* params)
{
    int i,j;
	NClist* values;
    if(params == NULL) return;
    for(i=0;i<nclistlength(params);i+=2) {
      char* s = (char*)nclistget(params,i);
      if(s != NULL) free((void*)s);
      values = (NClist*)nclistget(params,i+1);
      for(j=0;j<nclistlength(values);j++) {
        s = (char*)nclistget(values,j);
        if(s != NULL) free((void*)s);
      }
      nclistfree(values);
    }
    nclistfree(params);
}

void
nc_urlsetprotocol(NC_URL* ncurl,const char* newprotocol)
{
    if(ncurl != NULL) {
    if(ncurl->protocol != NULL) free(ncurl->protocol);
    ncurl->protocol = nulldup(newprotocol);
    }
}


#ifdef IGNORE
/*
Delete the entry.
return value = 1 => found and deleted;
0 => param not found
*/
static int
nc_urlparamdelete(NClist* params, const char* clientparam)
{
    int i,found = 0;
    if(params == NULL || clientparam == NULL) return 0;
    for(i=0;i<nclistlength(params);i+=2) {
    char* name = (char*)nclistget(params,i);
    if(strcmp(clientparam,name)==0) {found=1; break;}
    }
    if(found) {
    nclistremove(params,i+1); /* remove value */
    nclistremove(params,i); /* remove name */
    }
    return found;
}

/*
Replace new client param (name,value);
return value = 1 => replacement performed
0 => insertion performed
*/
static int
nc_urlparamreplace(NClist* params, const char* clientparam, const char* value)
{
    int i;
    if(params == NULL || clientparam == NULL) return 0;
    for(i=0;i<nclistlength(params);i+=2) {
    char* name = (char*)nclistget(params,i);
    if(strcmp(clientparam,name)==0) {
    nclistinsert(params,i+1,(ncelem)nulldup(value));
    return 1;
    }
    }
    nc_urlparaminsert(params,clientparam,value);
    return 0;
}

/*
Insert new client param (name,value);
return value = 1 => not already defined
0 => param already defined (no change)
*/
static int
nc_urlparaminsert(NClist* params, const char* clientparam, const char* value)
{
    int i;
    if(params == NULL || clientparam == NULL) return 0;
    for(i=0;i<nclistlength(params);i+=2) {
    char* name = (char*)nclistget(params,i);
    if(strcmp(clientparam,name)==0) return 0;
    }
    /* not found, append */
    nclistpush(params,(ncelem)strdup(clientparam));
    nclistpush(params,(ncelem)nulldup(value));
    return 1;
}

#endif
