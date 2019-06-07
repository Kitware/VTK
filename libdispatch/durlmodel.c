/**
 * @file
 *
 * URL Model Management.
 *
 * These functions support inferring the format X implementation for urls.
 * It looks at various fragment (#...) pairs.
 *
 * Copyright 2018 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
*/

#include "config.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ncdispatch.h"
#include "ncwinpath.h"
#include "ncurlmodel.h"

/*
Define a table of legal mode string values.
Note that only cases that can currently
take URLs are included.
*/
static struct LEGALMODES {
    const char* tag;
    const int format; /* NC_FORMAT_XXX value */
    const int implementation; /* NC_FORMATX_XXX value */
    const int iosp; /* NC_IOSP_XXX value */
} legalmodes[] = {
/* Format X Implementation */
{"netcdf-3",NC_FORMAT_CLASSIC,NC_FORMATX_NC3,0},
{"classic",NC_FORMAT_CLASSIC,NC_FORMATX_NC3,0},
{"netcdf-4",NC_FORMAT_NETCDF4,NC_FORMATX_NC4,0},
{"enhanced",NC_FORMAT_NETCDF4,NC_FORMATX_NC4,0},
{"dap2",NC_FORMAT_CLASSIC,NC_FORMATX_DAP2,0},
{"dap4",NC_FORMAT_NETCDF4,NC_FORMATX_DAP4,0},
/* IO Handlers */
{"zarr",0,0,NC_IOSP_ZARR},
{NULL,0,0},
};

/* Define the known URL protocols and their interpretation */
static struct NCPROTOCOLLIST {
    char* protocol;
    char* substitute;
    int   implementation;
} ncprotolist[] = {
    {"http",NULL,0},
    {"https",NULL,0},
    {"file",NULL,0},
    {"dods","http",NC_FORMATX_DAP2},
    {"dap4","http",NC_FORMATX_DAP4},
    {NULL,NULL,0} /* Terminate search */
};

/* Parse a mode string at the commas nul terminate each tag */
static int
parseurlmode(const char* modestr0, char** listp)
{
    int stat = NC_NOERR;
    char* modestr = NULL;
    char* p = NULL;
    char* endp = NULL;

    /* Make modifiable copy */
    if((modestr=strdup(modestr0)) == NULL)
	{stat=NC_ENOMEM; goto done;}

    /* Split modestr at the commas or EOL */
    p = modestr;
    for(;;) {
        endp = strchr(p,',');
        if(endp == NULL) break;
	/* Null terminate each comma-separated string */
	*endp++ = '\0';
	p = endp;
    } 
    if(listp) *listp = modestr;
    modestr = NULL;

done:
    if(stat) {nullfree(modestr);}
    return stat;
}

/* Parse url fragment for format etc. */
static int
url_getmodel(const char* modestr, NCmode* model)
{
    int stat = NC_NOERR;
    char* args = NULL;
    char* p = NULL;

    model->format = 0;
    model->implementation = 0;

    if((stat=parseurlmode(modestr,&args))) goto done;
    p = args;
    for(;*p;) {
	struct LEGALMODES* legal = legalmodes;
	while(legal->tag) {
	    if(strcmp(legal->tag,p)==0) {
		if(model->format != 0 && legal->format != 0)
		    {stat = NC_EINVAL; goto done;}
		if(model->implementation != 0 && legal->implementation != 0)
		    {stat = NC_EINVAL; goto done;}
		if(model->iosp != 0 && legal->iosp != 0)
		    {stat = NC_EINVAL; goto done;}
		if(legal->format != 0) model->format = legal->format;
		if(legal->implementation != 0)
		    model->implementation = legal->implementation;
		if(legal->iosp != 0) model->iosp = legal->iosp;
	    }
        }
    }	

done:
    nullfree(args);
    return stat;    
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

/*
Fill in the model fields to degree possible.
Assumes that the path is known to be a url
*/

int
NC_urlmodel(const char* path, int cmode, char** newurl, NCmode* model)
{
    int stat = NC_NOERR;
    int found = 0;
    struct NCPROTOCOLLIST* protolist;
    NCURI* url = NULL;
    const char** fragp = NULL;

    if(path == NULL) return 0;

    /* Parse the url */
    if(ncuriparse(path,&url) != NCU_OK)
	return NC_EINVAL; /* Not parseable as url */

    /* Look up the protocol */
    for(found=0,protolist=ncprotolist;protolist->protocol;protolist++) {
        if(strcmp(url->protocol,protolist->protocol) == 0) {
	    found = 1;
	    break;
	}
    }
    if(found) {
	model->implementation = protolist->implementation;
	/* Substitute the protocol in any case */
	if(protolist->substitute) ncurisetprotocol(url,protolist->substitute);
    } else
	{stat = NC_EINVAL; goto done;} /* Again, does not look like a url */

    /* Iterate over the url fragment parameters */
    for(fragp=ncurifragmentparams(url);fragp && *fragp;fragp+=2) {
	const char* name = fragp[0];
	const char* value = fragp[1];
	if(strcmp(name,"protocol")==0)
	    name = value;
	if(strcasecmp(name,"dap2") == 0) {
	    model->format = NC_FORMAT_NC3;	    
	    model->implementation = NC_FORMATX_DAP2;	    
	    /* No need to set iosp field */
	} else if(strcasecmp(name,"dap4") == 0) {
	    model->format = NC_FORMAT_NETCDF4;
	    model->implementation = NC_FORMATX_DAP4;
	    /* No need to set iosp field */
	} else if(strcmp(name,"mode")==0) {
	    if((stat = url_getmodel(value,model))) goto done;
	}
    }

    if(model->implementation == 0) {/* Last resort: use the cmode */
        /* If mode specifies netcdf-4, then this is assumed to be dap4 */
        if(cmode & NC_NETCDF4) {
	    model->implementation = NC_FORMATX_DAP4;
        } else {/* Default is DAP2 */
	    model->implementation = NC_FORMATX_DAP2;
	}
    }
    
    if(model->implementation == 0) goto done; /* could not interpret */
    switch (model->implementation) {
    case NC_FORMATX_NC3:
	model->format = NC_FORMAT_NC3;
	break;	
    case NC_FORMATX_NC4:
	model->format = NC_FORMAT_NETCDF4;
	break;	
    case NC_FORMATX_DAP2:
	model->format = NC_FORMAT_NC3;
	break;	
    case NC_FORMATX_DAP4:
	model->format = NC_FORMAT_NETCDF4;
	break;	
    case NC_FORMATX_ZARR:
	model->format = NC_FORMAT_NETCDF4;
	break;	
    default:
	stat = NC_EINVAL;
	goto done;
    }

done:
    if(newurl)
	*newurl = ncuribuild(url,NULL,NULL,NCURIALL);
    if(url) ncurifree(url);
    return stat;
}

/* return 1 if path looks like a url; 0 otherwise */
int
NC_testurl(const char* path)
{
    int isurl = 0;
    NCURI* tmpurl = NULL;

    if(path == NULL) return 0;

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
