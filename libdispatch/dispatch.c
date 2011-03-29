#include "ncdispatch.h"
#include "nc_url.h"

#define INITCOORD1 if(coord_one[0] != 1) {int i; for(i=0;i<NC_MAX_VAR_DIMS;i++) coord_one[i] = 1;}

/* Define the known protocols and their manipulations */
static struct NCPROTOCOLLIST {
    char* protocol;
    char* substitute;
    int   modelflags;
} ncprotolist[] = {
    {"http",NULL,0},
    {"https",NULL,0},
    {"file",NULL,NC_DISPATCH_NCD},
    {"dods","http",NC_DISPATCH_NCD},
    {"dodss","https",NC_DISPATCH_NCD},
    {"cdmr","http",NC_DISPATCH_NCR|NC_DISPATCH_NC4},
    {"cdmrs","https",NC_DISPATCH_NCR|NC_DISPATCH_NC4},
    {"cdmremote","http",NC_DISPATCH_NCR|NC_DISPATCH_NC4},
    {"cdmremotes","https",NC_DISPATCH_NCR|NC_DISPATCH_NC4},
    {NULL,NULL,0} /* Terminate search */
};

/*
static nc_type longtype = (sizeof(long) == sizeof(int)?NC_INT:NC_INT64);
static nc_type ulongtype = (sizeof(unsigned long) == sizeof(unsigned int)?NC_UINT:NC_UINT64);
*/

NC_Dispatch* NC3_dispatch_table = NULL;

#ifdef USE_NETCDF4
NC_Dispatch* NC4_dispatch_table = NULL;
#endif

#ifdef USE_DAP
NC_Dispatch* NCD3_dispatch_table = NULL;
#endif

#if defined(USE_DAP) && defined(USE_NETCDF4)
NC_Dispatch* NCD4_dispatch_table = NULL;
#endif

#if defined(USE_CDMREMOTE) && defined(USE_NETCDF4)
NC_Dispatch* NCCR_dispatch_table = NULL;
#endif

/* return 1 if path looks like a url; 0 otherwise */
int
NC_testurl(const char* path)
{
    int isurl = 0;
    NC_URL* tmpurl = NULL;
    char* p;

    if(path == NULL) return 0;

    /* find leading non-blank */
    for(p=path;*p;p++) {if(*p != ' ') break;}

    /* Do some initial checking to see if this looks like a file path */
    if(*p == '/') return 0; /* probably an absolute file path */

    /* Ok, try to parse as a url */
    if(nc_urlparse(path,&tmpurl) == NC_NOERR) {
	/* Do some extra testing to make sure this really is a url */
        /* Look for a knownprotocol */
        struct NCPROTOCOLLIST* protolist;
        for(protolist=ncprotolist;protolist->protocol;protolist++) {
	    if(strcmp(tmpurl->protocol,protolist->protocol) == 0) {
	        isurl=1;
		break;
	    }		
	}
	nc_urlfree(tmpurl);
	return isurl;
    }
    return 0;
}

/*
Return the OR of some of the NC_DISPATCH flags
Assumes that the path is known to be a url
*/

int
NC_urlmodel(const char* path)
{
    int model = 0;
    NC_URL* tmpurl = NULL;
    struct NCPROTOCOLLIST* protolist;

    if(nc_urlparse(path,&tmpurl) != NC_NOERR) goto done;

    /* Look at any prefixed parameters */
    if(nc_urllookup(tmpurl,"netcdf4")
       || nc_urllookup(tmpurl,"netcdf-4")) {
	model = (NC_DISPATCH_NC4|NC_DISPATCH_NCD);
    } else if(nc_urllookup(tmpurl,"netcdf3")
              || nc_urllookup(tmpurl,"netcdf-3")) {
	model = (NC_DISPATCH_NC3|NC_DISPATCH_NCD);
    } else if(nc_urllookup(tmpurl,"cdmremote")
	      || nc_urllookup(tmpurl,"cdmr")) {
	model = (NC_DISPATCH_NCR|NC_DISPATCH_NC4);
    }

    /* Now look at the protocol */
    for(protolist=ncprotolist;protolist->protocol;protolist++) {
	if(strcmp(tmpurl->protocol,protolist->protocol) == 0) {
	    model |= protolist->modelflags;
	    if(protolist->substitute)
	        nc_urlsetprotocol(tmpurl,protolist->substitute);	
	    break;	    
	}
    }	
    /* Force NC_DISPATCH_NC3 if necessary */
    if((model & NC_DISPATCH_NC4) == 0)
	model |= (NC_DISPATCH_NC3 | NC_DISPATCH_NCD);

done:
    nc_urlfree(tmpurl);
    return model;
}

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

/* Overlay by treating the tables as arrays of void*.
   Overlay rules are:
        overlay    base    merge
        -------    ----    -----
          null     null     null
          null      y        y
           x       null      x
           x        y        x
*/

int
NC_dispatch_overlay(const NC_Dispatch* overlay, const NC_Dispatch* base, NC_Dispatch* merge)
{
    void** voverlay = (void**)overlay;
    void** vmerge;
    int i, count = sizeof(NC_Dispatch) / sizeof(void*);
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
