#include "ncdispatch.h"
#include "ncuri.h"

/* Define vectors of zeros and ones for use with various nc_get_varX function*/
size_t nc_sizevector0[NC_MAX_VAR_DIMS];
size_t nc_sizevector1[NC_MAX_VAR_DIMS];
ptrdiff_t nc_ptrdiffvector1[NC_MAX_VAR_DIMS];
size_t NC_coord_zero[NC_MAX_VAR_DIMS];
size_t NC_coord_one[NC_MAX_VAR_DIMS];

/* Define the known protocols and their manipulations */
static struct NCPROTOCOLLIST {
    char* protocol;
    char* substitute;
    int   model;
} ncprotolist[] = {
    {"http",NULL,0},
    {"https",NULL,0},
    {"file",NULL,0},
    {"dods","http",NC_FORMATX_DAP2},
    {"dodss","https",NC_FORMATX_DAP2},
    {"dap4","http",NC_FORMATX_DAP4},
    {"dap4s","https",NC_FORMATX_DAP4},
    {NULL,NULL,0} /* Terminate search */
};

/*
static nc_type longtype = (sizeof(long) == sizeof(int)?NC_INT:NC_INT64);
static nc_type ulongtype = (sizeof(unsigned long) == sizeof(unsigned int)?NC_UINT:NC_UINT64);
*/

/* Allow dispatch to do general initialization and finalization */
int
NCDISPATCH_initialize(void)
{
    int status = NC_NOERR;
    int i;
    for(i=0;i<NC_MAX_VAR_DIMS;i++) {
	nc_sizevector0[i] = 0;
        nc_sizevector1[i] = 1;
        nc_ptrdiffvector1[i] = 1;
    }
    for(i=0;i<NC_MAX_VAR_DIMS;i++) {
	NC_coord_one[i] = 1;
	NC_coord_zero[i] = 0;
    }
    return status;
}

int
NCDISPATCH_finalize(void)
{
    int status = NC_NOERR;
    return status;
}

/* return 1 if path looks like a url; 0 otherwise */
int
NC_testurl(const char* path)
{
    int isurl = 0;
    NCURI* tmpurl = NULL;
    char* p;

    if(path == NULL) return 0;

    /* find leading non-blank */
    for(p=(char*)path;*p;p++) {if(*p != ' ') break;}

    /* Do some initial checking to see if this looks like a file path */
    if(*p == '/') return 0; /* probably an absolute file path */

    /* Ok, try to parse as a url */
    if(ncuriparse(path,&tmpurl)==NCU_OK) {
	/* Do some extra testing to make sure this really is a url */
        /* Look for a known/accepted protocol */
        struct NCPROTOCOLLIST* protolist;
        for(protolist=ncprotolist;protolist->protocol;protolist++) {
	    if(strcmp(tmpurl->protocol,protolist->protocol) == 0) {
	        isurl=1;
		break;
	    }
	}
	ncurifree(tmpurl);
	return isurl;
    }
    return 0;
}

/*
Return an NC_FORMATX_... value.
Assumes that the path is known to be a url
*/

int
NC_urlmodel(const char* path, int mode, char** newurl)
{
    int found, model = 0;
    struct NCPROTOCOLLIST* protolist;
    NCURI* url = NULL;
    char* p;

    if(path == NULL) return 0;

    /* find leading non-blank */
    for(p=(char*)path;*p;p++) {if(*p != ' ') break;}

    /* Do some initial checking to see if this looks like a file path */
    if(*p == '/') return 0; /* probably an absolute file path */

    /* Parse the url */
    if(ncuriparse(path,&url) != NCU_OK)
	goto fail; /* Not parseable as url */

    /* Look up the protocol */
    for(found=0,protolist=ncprotolist;protolist->protocol;protolist++) {
        if(strcmp(url->protocol,protolist->protocol) == 0) {
	    found = 1;
	    break;
	}
    }    
    if(found) {
	model = protolist->model;
	/* Substitute the protocol in any case */
	if(protolist->substitute) ncurisetprotocol(url,protolist->substitute);
    } else
	goto fail; /* Again, does not look like a url */

    if(model != NC_FORMATX_DAP2 && model != NC_FORMATX_DAP4) {
        /* Look for and of the following params:
  	   "dap2", "protocol=dap2", "dap4", "protocol=dap4" */
	const char* proto = NULL;
	const char* match = NULL;
	if((proto=ncurilookup(url,"protocol")) == NULL) proto = NULL;
	if(proto == NULL) proto = "";
	if((match=ncurilookup(url,"dap2")) != NULL || strcmp(proto,"dap2") == 0)
            model = NC_FORMATX_DAP2;
	else if((match=ncurilookup(url,"dap4")) != NULL || strcmp(proto,"dap4") == 0)
            model = NC_FORMATX_DAP4;
	else 
	model = 0; /* Still don't know */
    }
    if(model == 0) {/* Last resort: use the mode */
        /* If mode specifies netcdf-4, then this is assumed to be dap4 */
        if(mode & NC_NETCDF4)
	    model = NC_FORMATX_DAP4;
        else
            model = NC_FORMATX_DAP2; /* Default */
    }
    if(newurl)
	*newurl = ncuribuild(url,NULL,NULL,NCURIALL);
    return model;
fail:
    if(url) ncurifree(url);
    return 0;
}

/**************************************************/
/**
 * Provide a hidden interface to allow utilities
 * to check if a given path name is really an ncdap3 url.
 * If no, put null in basenamep, else put basename of the url
 * minus any extension into basenamep; caller frees.
 * Return 1 if it looks like a url, 0 otherwise.
 */

int
nc__testurl(const char* path, char** basenamep)
{
    int stat = NC_NOERR;
    NCURI* uri;
    int ok = 0;
    if(ncuriparse(path,&uri) == NCU_OK) {
	char* slash = (uri->path == NULL ? NULL : strrchr(uri->path, '/'));
	char* dot;
	if(slash == NULL) slash = (char*)path; else slash++;
        slash = nulldup(slash);
        if(slash == NULL)
            dot = NULL;
        else
            dot = strrchr(slash, '.');
        if(dot != NULL &&  dot != slash) *dot = '\0';
	if(basenamep)
            *basenamep=slash;
        else if(slash)
            free(slash);
        ncurifree(uri);
	ok = 1;
    }
    return ok;
}
/**************************************************/

#ifdef OBSOLETE
/* Override dispatch table management */
static NC_Dispatch* NC_dispatch_override = NULL;

/* Override dispatch table management */
NC_Dispatch*
NC_get_dispatch_override(void) {
    return NC_dispatch_override;
}

void NC_set_dispatch_override(NC_Dispatch* d)
{
    NC_dispatch_override = d;
}
#endif

/* OBSOLETE
   Overlay by treating the tables as arrays of void*.
   Overlay rules are:
        overlay    base    merge
        -------    ----    -----
          null     null     null
          null      y        y
           x       null      x
           x        y        x
*/

#ifdef OBSOLETE
int
NC_dispatch_overlay(const NC_Dispatch* overlay, const NC_Dispatch* base, NC_Dispatch* merge)
{
    void** voverlay = (void**)overlay;
    void** vmerge;
    int i;
    size_t count = sizeof(NC_Dispatch) / sizeof(void*);
    /* dispatch table must be exact multiple of sizeof(void*) */
    assert(count * sizeof(void*) == sizeof(NC_Dispatch));
    *merge = *base;
    vmerge = (void**)merge;
    for(i=0;i<count;i++) {
        if(voverlay[i] == NULL) continue;
        vmerge[i] = voverlay[i];
    }
    /* Finally, the merge model should always be the overlay model */
    merge->model = overlay->model;
    return NC_NOERR;
}
#endif
