/**
 * @file
 *
 * Infer as much as possible from the omode + path.
 * Rewrite the path to a canonical form.
 *
 * Copyright 2018 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
*/

#include "config.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ncdispatch.h"
#include "ncpathmgr.h"
#include "netcdf_mem.h"
#include "fbits.h"
#include "ncbytes.h"
#include "nclist.h"
#include "nclog.h"
#ifdef ENABLE_BYTERANGE
#include "nchttp.h"
#endif

#undef DEBUG

/* If Defined, then use only stdio for all magic number io;
   otherwise use stdio or mpio as required.
 */
#undef USE_STDIO

/**
Sort info for open/read/close of
file when searching for magic numbers
*/
struct MagicFile {
    const char* path;
    struct NCURI* uri;
    int omode;
    NCmodel* model;
    long long filelen;
    int use_parallel;
    void* parameters; /* !NULL if inmemory && !diskless */
    FILE* fp;
#ifdef USE_PARALLEL
    MPI_File fh;
#endif
#ifdef ENABLE_BYTERANGE
    char* curlurl; /* url to use with CURLOPT_SET_URL */
    NC_HTTP_STATE* state;
#endif
};

/** @internal Magic number for HDF5 files. To be consistent with
 * H5Fis_hdf5, use the complete HDF5 magic number */
static char HDF5_SIGNATURE[MAGIC_NUMBER_LEN] = "\211HDF\r\n\032\n";

#define modelcomplete(model) ((model)->impl != 0)

#ifdef DEBUG
static void dbgflush(void)
{
    fflush(stdout);
    fflush(stderr);
}

static void
fail(int err)
{
    return;
}

static int
check(int err)
{
    if(err != NC_NOERR)
	fail(err);
    return err;
}
#else
#define check(err) (err)
#endif

/*
Define a table of "mode=" string values
from which the implementation can be inferred.
Note that only cases that can currently
take URLs are included.
*/
static struct FORMATMODES {
    const char* tag;
    const int impl; /* NC_FORMATX_XXX value */
    const int format; /* NC_FORMAT_XXX value */
} formatmodes[] = {
{"dap2",NC_FORMATX_DAP2,NC_FORMAT_CLASSIC},
{"dap4",NC_FORMATX_DAP4,NC_FORMAT_NETCDF4},
{"netcdf-3",NC_FORMATX_NC3,0}, /* Might be e.g. cdf5 */
{"classic",NC_FORMATX_NC3,0}, /* ditto */
{"netcdf-4",NC_FORMATX_NC4,NC_FORMAT_NETCDF4},
{"enhanced",NC_FORMATX_NC4,NC_FORMAT_NETCDF4},
{"udf0",NC_FORMATX_UDF0,NC_FORMAT_NETCDF4},
{"udf1",NC_FORMATX_UDF1,NC_FORMAT_NETCDF4},
{"nczarr",NC_FORMATX_NCZARR,NC_FORMAT_NETCDF4},
{"zarr",NC_FORMATX_NCZARR,NC_FORMAT_NETCDF4},
{NULL,0},
};

/* For some reason, compiler complains */
static const struct MACRODEF {
    char* name;
    char* defkey;
    char* defvalue;
} macrodefs[] = {
{"zarr","mode","nczarr,zarr"},
{"dap2","mode","dap2"},
{"dap4","mode","dap4"},
{"s3","mode","nczarr,s3"},
{"bytes","mode","bytes"},
{NULL,NULL,NULL}
};

/* Map FORMATX to readability to get magic number */
static struct Readable {
    int impl;
    int readable;
} readable[] = {
{NC_FORMATX_NC3,1},
{NC_FORMATX_NC_HDF5,1},
{NC_FORMATX_NC_HDF4,1},
{NC_FORMATX_PNETCDF,1},
{NC_FORMATX_DAP2,0},
{NC_FORMATX_DAP4,0},
{NC_FORMATX_UDF0,0},
{NC_FORMATX_UDF1,0},
{NC_FORMATX_NCZARR,0}, /* eventually make readable */
{0,0},
};

/* Define the known URL protocols and their interpretation */
static struct NCPROTOCOLLIST {
    const char* protocol;
    const char* substitute;
    const char* fragments; /* arbitrary fragment arguments */
} ncprotolist[] = {
    {"http",NULL,NULL},
    {"https",NULL,NULL},
    {"file",NULL,NULL},
    {"dods","http","dap2"},
    {"dap4","http","dap4"},
    {"s3","http","s3"},
    {NULL,NULL,NULL} /* Terminate search */
};

/* Forward */
static int NC_omodeinfer(int useparallel, int omode, NCmodel*);
static int check_file_type(const char *path, int omode, int use_parallel, void *parameters, NCmodel* model, NCURI* uri);
static int processuri(const char* path, NCURI** urip, NClist* fraglist);
static int processmacros(NClist** fraglistp);
static char* envvlist2string(NClist* pairs, const char*);
static void set_default_mode(int* cmodep);
static int parseonchar(const char* s, int ch, NClist* segments);

static int openmagic(struct MagicFile* file);
static int readmagic(struct MagicFile* file, long pos, char* magic);
static int closemagic(struct MagicFile* file);
static int NC_interpret_magic_number(char* magic, NCmodel* model);
#ifdef DEBUG
static void printmagic(const char* tag, char* magic,struct MagicFile*);
static void printlist(NClist* list, const char* tag);
#endif
static int isreadable(NCmodel*);
static char* list2string(NClist*);
static int parsepair(const char* pair, char** keyp, char** valuep);

/*
If the path looks like a URL, then parse it, reformat it.
*/
static int
processuri(const char* path, NCURI** urip, NClist* fraglenv)
{
    int stat = NC_NOERR;
    int found = 0;
    NClist* tmp = NULL;
    struct NCPROTOCOLLIST* protolist;
    NCURI* uri = NULL;
    size_t pathlen = strlen(path);
    char* str = NULL;
    const char** ufrags;
    const char** p;

    if(path == NULL || pathlen == 0) {stat = NC_EURL; goto done;}

    /* Defaults */
    if(urip) *urip = NULL;

    if(ncuriparse(path,&uri)) goto done; /* not url */

    /* Look up the protocol */
    for(found=0,protolist=ncprotolist;protolist->protocol;protolist++) {
        if(strcmp(uri->protocol,protolist->protocol) == 0) {
	    found = 1;
	    break;
	}
    }
    if(!found)
	{stat = NC_EINVAL; goto done;} /* unrecognized URL form */

    /* process the corresponding fragments for that protocol */ 
    if(protolist->fragments != NULL) {
	int i;
	tmp = nclistnew();
	if((stat = parseonchar(protolist->fragments,'&',tmp))) goto done;
	for(i=0;i<nclistlength(tmp);i++) {
	    char* key=NULL;
    	    char* value=NULL;
	    if((stat = parsepair(nclistget(tmp,i),&key,&value))) goto done;
	    if(value == NULL) value = strdup("");
	    nclistpush(fraglenv,key);
    	    nclistpush(fraglenv,value);
	}
	nclistfreeall(tmp); tmp = NULL;
    }
    
    /* Substitute the protocol in any case */
    if(protolist->substitute) ncurisetprotocol(uri,protolist->substitute);

    /* capture the fragments of the url */
    ufrags = ncurifragmentparams(uri);
    if(ufrags != NULL) {
        for(p=ufrags;*p;p+=2) {
	    const char* key = p[0];
	    const char* value = p[1];
	    nclistpush(fraglenv,nulldup(key));
	    value = (value==NULL?"":value);
	    nclistpush(fraglenv,strdup(value));
	}
    }
    if(urip) {
	*urip = uri;
	uri = NULL;
    }

done:
    nclistfreeall(tmp);
    nullfree(str);
    if(uri != NULL) ncurifree(uri);
    return check(stat);
}

/* Split a key=value pair */
static int
parsepair(const char* pair, char** keyp, char** valuep)
{
    const char* p;
    char* key = NULL;
    char* value = NULL;

    if(pair == NULL)
        return NC_EINVAL; /* empty pair */
    if(pair[0] == '\0' || pair[0] == '=')
        return NC_EINVAL; /* no key */
    p = strchr(pair,'=');
    if(p == NULL) {
	value = NULL;
	key = strdup(pair);
    } else {
	ptrdiff_t len = (p-pair);
	if((key = malloc(len+1))==NULL) return NC_ENOMEM;
	memcpy(key,pair,len);
	key[len] = '\0';
	if(p[1] == '\0')
	    value = NULL;
	else
	    value = strdup(p+1);
    }
    if(keyp) {*keyp = key; key = NULL;};
    if(valuep) {*valuep = value; value = NULL;};
    nullfree(key);
    nullfree(value);
    return NC_NOERR;
}

#if 0
static int
parseurlmode(const char* modestr, NClist* list)
{
    int stat = NC_NOERR;
    const char* p = NULL;
    const char* endp = NULL;

    if(modestr == NULL || *modestr == '\0') goto done;

    /* Split modestr at the commas or EOL */
    p = modestr;
    for(;;) {
	char* s;
	ptrdiff_t slen;
	endp = strchr(p,',');
	if(endp == NULL) endp = p + strlen(p);
	slen = (endp - p);
	if((s = malloc(slen+1)) == NULL) {stat = NC_ENOMEM; goto done;}
	memcpy(s,p,slen);
	s[slen] = '\0';
	nclistpush(list,s);
	if(*endp == '\0') break;
	p = endp+1;
    }

done:
    return check(stat);
}
#endif

/* Split a string at a given char */
static int
parseonchar(const char* s, int ch, NClist* segments)
{
    int stat = NC_NOERR;
    const char* p = NULL;
    const char* endp = NULL;

    if(s == NULL || *s == '\0') goto done;

    p = s;
    for(;;) {
	char* q;
	ptrdiff_t slen;
	endp = strchr(p,ch);
	if(endp == NULL) endp = p + strlen(p);
	slen = (endp - p);
	if((q = malloc(slen+1)) == NULL) {stat = NC_ENOMEM; goto done;}
	memcpy(q,p,slen);
	q[slen] = '\0';
	nclistpush(segments,q);
	if(*endp == '\0') break;
	p = endp+1;
    }

done:
    return check(stat);
}

/* Convert a key,value envv pairlist into a delimited string*/
static char*
envvlist2string(NClist* envv, const char* delim)
{
    int i;
    NCbytes* buf = NULL;
    char* result = NULL;

    if(envv == NULL || nclistlength(envv) == 0) return NULL;
    buf = ncbytesnew();
    for(i=0;i<nclistlength(envv);i+=2) {
	const char* key = nclistget(envv,i);
	const char* val = nclistget(envv,i+1);
	if(key == NULL || strlen(key) == 0) continue;
	assert(val != NULL);
	if(i > 0) ncbytescat(buf,"&");
	ncbytescat(buf,key);
	if(val != NULL && val[0] != '\0') {
	    ncbytescat(buf,"=");
	    ncbytescat(buf,val);
	}
    }
    result = ncbytesextract(buf);
    ncbytesfree(buf);
    return result;
}

/* Convert a list into a comma'd string */
static char*
list2string(NClist* list)
{
    int i;
    NCbytes* buf = NULL;
    char* result = NULL;

    if(list == NULL || nclistlength(list)==0) return strdup("");
    buf = ncbytesnew();
    for(i=0;i<nclistlength(list);i++) {
	const char* m = nclistget(list,i);
	if(m == NULL || strlen(m) == 0) continue;
	if(i > 0) ncbytescat(buf,",");
	ncbytescat(buf,m);
    }
    result = ncbytesextract(buf);
    ncbytesfree(buf);
    if(result == NULL) result = strdup("");
    return result;
}

/* Given a mode= argument, fill in the impl */
static int
processmodearg(const char* arg, NCmodel* model)
{
    int stat = NC_NOERR;
    struct FORMATMODES* format = formatmodes;
    for(;format->tag;format++) {
	if(strcmp(format->tag,arg)==0) {
            model->impl = format->impl;
	    if(format->format != 0) model->format = format->format;
	}
    }
    return check(stat);
}

/* Given an envv fragment list, do macro replacement */
static int
processmacros(NClist** fraglenvp)
{
    int stat = NC_NOERR;
    const struct MACRODEF* macros = NULL;
    NClist*  fraglenv = NULL;
    NClist* expanded = NULL;

    if(fraglenvp == NULL || nclistlength(*fraglenvp) == 0) goto done;
    fraglenv = *fraglenvp;
    expanded = nclistnew();    
    while(nclistlength(fraglenv) > 0) {
	int found = 0;
	char* key = NULL;
	char* value = NULL;
	key = nclistremove(fraglenv,0); /* remove from changing front */
	value = nclistremove(fraglenv,0); /* remove from changing front */
	if(strlen(value) == 0) { /* must be a singleton  */
            for(macros=macrodefs;macros->name;macros++) {
                if(strcmp(macros->name,key)==0) {
		    nclistpush(expanded,strdup(macros->defkey));
	            nclistpush(expanded,strdup(macros->defvalue));
		    found = 1;		    
		    break;
	        }
	    }
	}
	if(!found) {/* pass thru */
	    nclistpush(expanded,strdup(key));
    	    nclistpush(expanded,strdup(value));
	}
	nullfree(key);
	nullfree(value);
    }
    *fraglenvp = expanded; expanded = NULL;

done:
    nclistfreeall(expanded);
    nclistfreeall(fraglenv);
    return check(stat);
}

static int
mergekey(NClist** valuesp)
{
    int i,j;
    int stat = NC_NOERR;
    NClist* values = *valuesp;
    NClist* allvalues = nclistnew();
    NClist* newvalues = nclistnew();
    char* value = NULL;

    for(i=0;i<nclistlength(values);i++) {
	char* val1 = nclistget(values,i);
	/* split on commas and put pieces into allvalues */
	if((stat=parseonchar(val1,',',allvalues))) goto done;
    }
    /* Remove duplicates and "" */
    while(nclistlength(allvalues) > 0) {
	value = nclistremove(allvalues,0);
	if(strlen(value) == 0) {
	    nullfree(value); value = NULL;
	} else {
	    for(j=0;j<nclistlength(newvalues);j++) {
	        char* candidate = nclistget(newvalues,j);
	        if(strcasecmp(candidate,value)==0)
	            {nullfree(value); value = NULL; break;}
	     }
	} 
	if(value != NULL) {nclistpush(newvalues,value); value = NULL;}
    }
    /* Make sure to have at least 1 value */
    if(nclistlength(newvalues)==0) nclistpush(newvalues,strdup(""));
    *valuesp = values; values = NULL;

done:
    nclistfree(allvalues);
    nclistfreeall(values);
    nclistfreeall(newvalues);
    return check(stat);
}

static int
lcontains(NClist* l, const char* key0)
{
    int i;
    for(i=0;i<nclistlength(l);i++) {
        const char* key1 = nclistget(l,i);
	if(strcasecmp(key0,key1)==0) return 1;
    }
    return 0;
}

/* Warning values should not use nclistfreeall */
static void
collectvaluesbykey(NClist* fraglenv, const char* key, NClist* values)
{
    int i;
    /* collect all the values with the same key (including this one) */
    for(i=0;i<nclistlength(fraglenv);i+=2) {
        const char* key2 = nclistget(fraglenv,i);
        if(strcasecmp(key,key2)==0) {
	    const char* value2 = nclistget(fraglenv,i+1);
	    nclistpush(values,value2); value2 = NULL;
	}
    }
}

/* Warning allkeys should not use nclistfreeall */
static void
collectallkeys(NClist* fraglenv, NClist* allkeys)
{
    int i;
    /* collect all the distinct keys */
    for(i=0;i<nclistlength(fraglenv);i+=2) {
	char* key = nclistget(fraglenv,i);
	if(!lcontains(allkeys,key)) {
	    nclistpush(allkeys,key);
	}
    }
}

/* Given a fragment envv list, coalesce duplicate keys and remove duplicate values*/
static int
cleanfragments(NClist** fraglenvp)
{
    int i,stat = NC_NOERR;
    NClist*  fraglenv = NULL;
    NClist* tmp = NULL;
    NClist* allkeys = NULL;
    NClist* newlist = NULL;
    NCbytes* buf = NULL;
    char* key = NULL;
    char* value = NULL;

    if(fraglenvp == NULL || nclistlength(*fraglenvp) == 0) return NC_NOERR;
    fraglenv = *fraglenvp; /* take control of this list */
    *fraglenvp = NULL;
    newlist = nclistnew();
    buf = ncbytesnew();
    allkeys = nclistnew();
    tmp = nclistnew();    

    /* collect all unique keys */
    collectallkeys(fraglenv,allkeys);
    /* Collect all values for same key across all fragments */ 
    for(i=0;i<nclistlength(allkeys);i++) {
	key = nclistget(allkeys,i);
	collectvaluesbykey(fraglenv,key,tmp);
	/* merge the key values, remove duplicate */
	if((stat=mergekey(&tmp))) goto done;
        /* Construct key,value pair and insert into newlist */
	key = strdup(key);
	nclistpush(newlist,key);
	value = list2string(tmp);
	nclistpush(newlist,value);
	nclistclear(tmp);
    }
    *fraglenvp = newlist; newlist = NULL;
done:
    nclistfree(allkeys);
    nclistfree(tmp);
    ncbytesfree(buf);
    nclistfreeall(fraglenv);
    nclistfreeall(newlist);
    return check(stat);
}

/* process non-mode fragment keys in case they hold significance; currently not */
static int
processfragmentkeys(const char* key, const char* value, NCmodel* model)
{
    return NC_NOERR;    
}

/*
Infer from the mode + useparallel
only call if iscreate or file is not easily readable.
*/
static int
NC_omodeinfer(int useparallel, int cmode, NCmodel* model)
{
    int stat = NC_NOERR;

    /* If no format flags are set, then use default */
    if(!fIsSet(cmode,NC_FORMAT_ALL))
	set_default_mode(&cmode);

    /* Process the cmode; may override some already set flags. The
     * user-defined formats must be checked first. They may choose to
     * use some of the other flags, like NC_NETCDF4, so we must fist
     * check NC_UDF0 and NC_UDF1 before checking for any other
     * flag. */
    if(fIsSet(cmode,(NC_UDF0|NC_UDF1))) {
	model->format = NC_FORMAT_NETCDF4;
        if(fIsSet(cmode,NC_UDF0)) {
	    model->impl = NC_FORMATX_UDF0;
	} else {
	    model->impl = NC_FORMATX_UDF1;
	}
	goto done;
    }

    if(fIsSet(cmode,NC_64BIT_OFFSET)) {
	model->impl = NC_FORMATX_NC3;
	model->format = NC_FORMAT_64BIT_OFFSET;
        goto done;
    }

    if(fIsSet(cmode,NC_64BIT_DATA)) {
	model->impl = NC_FORMATX_NC3;
	model->format = NC_FORMAT_64BIT_DATA;
        goto done;
    }

    if(fIsSet(cmode,NC_NETCDF4)) {
	model->impl = NC_FORMATX_NC4;
        if(fIsSet(cmode,NC_CLASSIC_MODEL))
	    model->format = NC_FORMAT_NETCDF4_CLASSIC;
	else
	    model->format = NC_FORMAT_NETCDF4;
        goto done;
    }

    /* Default to classic model */
    model->format = NC_FORMAT_CLASSIC;
    model->impl = NC_FORMATX_NC3;

done:
    /* Apply parallel flag */
    if(useparallel) {
        if(model->impl == NC_FORMATX_NC3)
	    model->impl = NC_FORMATX_PNETCDF;
    }
    return check(stat);
}

/*
If the mode flags do not necessarily specify the
format, then default it by adding in appropriate flags.
*/

static void
set_default_mode(int* modep)
{
    int mode = *modep;
    int dfaltformat;

    dfaltformat = nc_get_default_format();
    switch (dfaltformat) {
    case NC_FORMAT_64BIT_OFFSET: mode |= NC_64BIT_OFFSET; break;
    case NC_FORMAT_64BIT_DATA: mode |= NC_64BIT_DATA; break;
    case NC_FORMAT_NETCDF4: mode |= NC_NETCDF4; break;
    case NC_FORMAT_NETCDF4_CLASSIC: mode |= (NC_NETCDF4|NC_CLASSIC_MODEL); break;
    case NC_FORMAT_CLASSIC: /* fall thru */
    default: break; /* default to classic */
    }    
    *modep = mode; /* final result */
}

/**************************************************/
/*
   Infer model for this dataset using some
   combination of cmode, path, and reading the dataset.
   See the documentation in docs/internal.dox.

@param path
@param omode
@param iscreate
@param useparallel
@param params
@param model
@param newpathp
*/

int
NC_infermodel(const char* path, int* omodep, int iscreate, int useparallel, void* params, NCmodel* model, char** newpathp)
{
    int i,stat = NC_NOERR;
    NCURI* uri = NULL;
    int omode = *omodep;
    NClist* fraglenv = nclistnew();
    NClist* modeargs = nclistnew();
    char* sfrag = NULL;
    const char* modeval = NULL;

    /* Phase 1:
       1. convert special protocols to http|https
       2. begin collecting fragments
    */
    if((stat = processuri(path, &uri, fraglenv))) goto done;

    if(uri != NULL) {
#ifdef DEBUG
	printlist(fraglenv,"processuri");
#endif

        /* Phase 2: Expand macros and add to fraglenv */
        if((stat = processmacros(&fraglenv))) goto done;
#ifdef DEBUG
	printlist(fraglenv,"processmacros");
#endif

        /* Phase 3: coalesce duplicate fragment keys and remove duplicate values */
        if((stat = cleanfragments(&fraglenv))) goto done;
#ifdef DEBUG
	printlist(fraglenv,"cleanfragments");
#endif

        /* Phase 4: Rebuild the url fragment and rebuilt the url */
        sfrag = envvlist2string(fraglenv,"&");        
        nclistfreeall(fraglenv); fraglenv = NULL;
#ifdef DEBUG
	fprintf(stderr,"frag final: %s\n",sfrag);
#endif
        ncurisetfragments(uri,sfrag);
        nullfree(sfrag); sfrag = NULL;

	/* rebuild the path */
        if(newpathp)
            *newpathp = ncuribuild(uri,NULL,NULL,NCURIALL);
#ifdef DEBUG
    fprintf(stderr,"newpath=|%s|\n",*newpathp); fflush(stderr);
#endif    

        /* Phase 5: Process the mode key to see if we can tell the formatx */
        modeval = ncurifragmentlookup(uri,"mode");        
        if(modeval != NULL) {
	    if((stat = parseonchar(modeval,',',modeargs))) goto done;
            for(i=0;i<nclistlength(modeargs);i++) {
        	const char* arg = nclistget(modeargs,i);
        	if((stat=processmodearg(arg,model))) goto done;
            }
	}

        /* Phase 6: Process the non-mode keys to see if we can tell the formatx */
	if(!modelcomplete(model)) {
	    const char** p = ncurifragmentparams(uri); /* envv format */
	    if(p != NULL) {
	        for(;*p;p++) {
		    const char* key = p[0];
		    const char* value = p[1];;
        	    if((stat=processfragmentkeys(key,value,model))) goto done;
	        }
	    }
	}

        /* Phase 6: Special case: if this is a URL, and there are no mode args
           and model.impl is still not defined, default to DAP2 */
        if(nclistlength(modeargs) == 0 && !modelcomplete(model)) {
	    model->impl = NC_FORMATX_DAP2;
	    model->format = NC_FORMAT_NC3;
        }
    } else {/* Not URL */
	if(*newpathp) *newpathp = NULL;
    }        

    /* Phase 7: mode inference from mode flags */
    /* The modeargs did not give us a model (probably not a URL).
       So look at the combination of mode flags and the useparallel flag */
    if(!modelcomplete(model)) {
        if((stat = NC_omodeinfer(useparallel,omode,model))) goto done;
    }

    /* Phase 6: Infer from file content, if possible;
       this has highest precedence, so it may override
       previous decisions. Note that we do this last
       because we need previously determined model info
       to guess if this file is readable.
    */
    if(!iscreate && isreadable(model)) {
	/* Ok, we need to try to read the file */
	if((stat = check_file_type(path, omode, useparallel, params, model, uri))) goto done;
    }

    /* Need a decision */
    if(!modelcomplete(model))
	{stat = NC_ENOTNC; goto done;}

    /* Force flag consistency */
    switch (model->impl) {
    case NC_FORMATX_NC4:
    case NC_FORMATX_NC_HDF4:
    case NC_FORMATX_DAP4:
    case NC_FORMATX_UDF0:
    case NC_FORMATX_UDF1:
    case NC_FORMATX_NCZARR:
	omode |= NC_NETCDF4;
	if(model->format == NC_FORMAT_NETCDF4_CLASSIC)
	    omode |= NC_CLASSIC_MODEL;
	break;
    case NC_FORMATX_NC3:
	omode &= ~NC_NETCDF4; /* must be netcdf-3 (CDF-1, CDF-2, CDF-5) */
	if(model->format == NC_FORMAT_64BIT_OFFSET) omode |= NC_64BIT_OFFSET;
	else if(model->format == NC_FORMAT_64BIT_DATA) omode |= NC_64BIT_DATA;
	break;
    case NC_FORMATX_PNETCDF:
	omode &= ~NC_NETCDF4; /* must be netcdf-3 (CDF-1, CDF-2, CDF-5) */
	if(model->format == NC_FORMAT_64BIT_OFFSET) omode |= NC_64BIT_OFFSET;
	else if(model->format == NC_FORMAT_64BIT_DATA) omode |= NC_64BIT_DATA;
	break;
    case NC_FORMATX_DAP2:
	omode &= ~(NC_NETCDF4|NC_64BIT_OFFSET|NC_64BIT_DATA|NC_CLASSIC_MODEL);
	break;
    default:
	{stat = NC_ENOTNC; goto done;}
    }

done:
    nullfree(sfrag);
    ncurifree(uri);
    nclistfreeall(modeargs);
    nclistfreeall(fraglenv);
    *omodep = omode; /* in/out */
    return check(stat);
}

static int
isreadable(NCmodel* model)
{
    struct Readable* r;
    /* Look up the protocol */
    for(r=readable;r->impl;r++) {
	if(model->impl == r->impl) return r->readable;
    }
    return 0;
}

#if 0
static char*
emptyify(char* s)
{
    if(s == NULL) s = strdup("");
    return strdup(s);
}

static const char*
nullify(const char* s)
{
    if(s != NULL && strlen(s) == 0)
        return NULL;
    return s;
}
#endif

/**************************************************/
/**************************************************/
/**
 * Provide a hidden interface to allow utilities
 * to check if a given path name is really a url.
 * If not, put null in basenamep, else put basename of the url path
 * minus any extension into basenamep; caller frees.
 * Return 1 if it looks like a url, 0 otherwise.
 */

int
nc__testurl(const char* path0, char** basenamep)
{
    NCURI* uri = NULL;
    int ok = 0;
    char* path = NULL;
    
    if(!ncuriparse(path0,&uri)) {
	char* p;
	char* q;
	path = strdup(uri->path);
	if(path == NULL||strlen(path)==0) goto done;
        p = strrchr(path, '/');
	if(p == NULL) p = path; else p++;
	q = strrchr(p,'.');
        if(q != NULL) *q = '\0';
	if(strlen(p) == 0) goto done;
	if(basenamep)
            *basenamep = strdup(p);
	ok = 1;
    }
done:
    ncurifree(uri);
    nullfree(path);
    return ok;
}



/**************************************************/
/**
 * @internal Given an existing file, figure out its format and return
 * that format value (NC_FORMATX_XXX) in model arg. Assume any path
 * conversion was already performed at a higher level.
 *
 * @param path File name.
 * @param flags
 * @param use_parallel
 * @param parameters
 * @param model Pointer that gets the model to use for the dispatch table.
 * @param version Pointer that gets version of the file.
 *
 * @return ::NC_NOERR No error.
 * @author Dennis Heimbigner
*/
static int
check_file_type(const char *path, int omode, int use_parallel,
		   void *parameters, NCmodel* model, NCURI* uri)
{
    char magic[NC_MAX_MAGIC_NUMBER_LEN];
    int status = NC_NOERR;
    struct MagicFile magicinfo;

    memset((void*)&magicinfo,0,sizeof(magicinfo));
    magicinfo.path = path; /* do not free */
    magicinfo.uri = uri; /* do not free */
    magicinfo.omode = omode;
    magicinfo.model = model; /* do not free */
    magicinfo.parameters = parameters; /* do not free */
#ifdef USE_STDIO
    magicinfo.use_parallel = 0;
#else
    magicinfo.use_parallel = use_parallel;
#endif

    if((status = openmagic(&magicinfo))) goto done;

    /* Verify we have a large enough file */
    if(magicinfo.filelen < (long long)MAGIC_NUMBER_LEN)
	{status = NC_ENOTNC; goto done;}
    if((status = readmagic(&magicinfo,0L,magic)) != NC_NOERR) {
	status = NC_ENOTNC;
	goto done;
    }

    /* Look at the magic number */
    if(NC_interpret_magic_number(magic,model) == NC_NOERR
	&& model->format != 0) {
        if (use_parallel && (model->format == NC_FORMAT_NC3 || model->impl == NC_FORMATX_NC3))
            /* this is called from nc_open_par() and file is classic */
            model->impl = NC_FORMATX_PNETCDF;
        goto done; /* found something */
    }

    /* Remaining case when implementation is an HDF5 file;
       search forward at starting at 512
       and doubling to see if we have HDF5 magic number */
    {
	long pos = 512L;
        for(;;) {
	    if((pos+MAGIC_NUMBER_LEN) > magicinfo.filelen)
		{status = NC_ENOTNC; goto done;}
            if((status = readmagic(&magicinfo,pos,magic)) != NC_NOERR)
	        {status = NC_ENOTNC; goto done; }
            NC_interpret_magic_number(magic,model);
            if(model->impl == NC_FORMATX_NC4) break;
	    /* double and try again */
	    pos = 2*pos;
        }
    }
done:
    closemagic(&magicinfo);
    return check(status);
}

/**
\internal
\ingroup datasets
Provide open, read and close for use when searching for magic numbers
*/
static int
openmagic(struct MagicFile* file)
{
    int status = NC_NOERR;

    if(fIsSet(file->omode,NC_INMEMORY)) {
	/* Get its length */
	NC_memio* meminfo = (NC_memio*)file->parameters;
        assert(meminfo != NULL);
	file->filelen = (long long)meminfo->size;
#ifdef ENABLE_BYTERANGE
    } else if(file->uri != NULL) {
	/* Construct a URL minus any fragment */
        file->curlurl = ncuribuild(file->uri,NULL,NULL,NCURISVC);
	/* Open the curl handle */
	if((status=nc_http_open(file->curlurl,&file->state,&file->filelen))) goto done;
#endif
    } else {
#ifdef USE_PARALLEL
        if (file->use_parallel) {
	    int retval;
	    MPI_Offset size;
            assert(file->parameters != NULL);
	    if((retval = MPI_File_open(((NC_MPI_INFO*)file->parameters)->comm,
                                   (char*)file->path,MPI_MODE_RDONLY,
                                   ((NC_MPI_INFO*)file->parameters)->info,
                                   &file->fh)) != MPI_SUCCESS) {
#ifdef MPI_ERR_NO_SUCH_FILE
		int errorclass;
		MPI_Error_class(retval, &errorclass);
		if (errorclass == MPI_ERR_NO_SUCH_FILE)
#ifdef NC_ENOENT
		    status = NC_ENOENT;
#else
		    status = errno;
#endif
		else
#endif
		    status = NC_EPARINIT;
		goto done;
	    }
	    /* Get its length */
	    if((retval=MPI_File_get_size(file->fh, &size)) != MPI_SUCCESS)
	        {status = NC_EPARINIT; goto done;}
	    file->filelen = (long long)size;
	} else
#endif /* USE_PARALLEL */
	{
	    if(file->path == NULL || strlen(file->path)==0)
	        {status = NC_EINVAL; goto done;}

#ifdef _WIN32
            file->fp = NCfopen(file->path, "rb");
#else
            file->fp = NCfopen(file->path, "r");
#endif
   	    if(file->fp == NULL)
	        {status = errno; goto done;}
  	    /* Get its length */
	    {
	        int fd = fileno(file->fp);
#ifdef _WIN32
		__int64 len64 = _filelengthi64(fd);
		if(len64 < 0)
		    {status = errno; goto done;}
		file->filelen = (long long)len64;
#else
		off_t size;
		size = lseek(fd, 0, SEEK_END);
		if(size == -1)
		    {status = errno; goto done;}
		file->filelen = (long long)size;
#endif
	    }
	    rewind(file->fp);
	  }
    }
done:
    return check(status);
}

static int
readmagic(struct MagicFile* file, long pos, char* magic)
{
    int status = NC_NOERR;
    memset(magic,0,MAGIC_NUMBER_LEN);
    if(fIsSet(file->omode,NC_INMEMORY)) {
	char* mempos;
	NC_memio* meminfo = (NC_memio*)file->parameters;
	if((pos + MAGIC_NUMBER_LEN) > meminfo->size)
	    {status = NC_EINMEMORY; goto done;}
	mempos = ((char*)meminfo->memory) + pos;
	memcpy((void*)magic,mempos,MAGIC_NUMBER_LEN);
#ifdef DEBUG
	printmagic("XXX: readmagic",magic,file);
#endif
#ifdef ENABLE_BYTERANGE
    } else if(file->uri != NULL) {
	NCbytes* buf = ncbytesnew();
	fileoffset_t start = (size_t)pos;
	fileoffset_t count = MAGIC_NUMBER_LEN;
	status = nc_http_read(file->state,file->curlurl,start,count,buf);
	if(status == NC_NOERR) {
	    if(ncbyteslength(buf) != count)
	        status = NC_EINVAL;
	    else
	        memcpy(magic,ncbytescontents(buf),count);
	}
	ncbytesfree(buf);
#endif
    } else {
#ifdef USE_PARALLEL
        if (file->use_parallel) {
	    MPI_Status mstatus;
	    int retval;
	    if((retval = MPI_File_read_at_all(file->fh, pos, magic,
			    MAGIC_NUMBER_LEN, MPI_CHAR, &mstatus)) != MPI_SUCCESS)
	        {status = NC_EPARINIT; goto done;}
	} else
#endif /* USE_PARALLEL */
	{
	    int count;
	    int i = fseek(file->fp,pos,SEEK_SET);
	    if(i < 0)
	        {status = errno; goto done;}
  	    for(i=0;i<MAGIC_NUMBER_LEN;) {/* make sure to read proper # of bytes */
	        count=fread(&magic[i],1,(size_t)(MAGIC_NUMBER_LEN-i),file->fp);
	        if(count == 0 || ferror(file->fp))
		    {status = errno; goto done;}
	        i += count;
	    }
	}
    }

done:
    if(file && file->fp) clearerr(file->fp);
    return check(status);
}

/**
 * Close the file opened to check for magic number.
 *
 * @param file pointer to the MagicFile struct for this open file.
 * @returns NC_NOERR for success
 * @returns NC_EPARINIT if there was a problem closing file with MPI
 * (parallel builds only).
 * @author Dennis Heimbigner
 */
static int
closemagic(struct MagicFile* file)
{
    int status = NC_NOERR;
    if(fIsSet(file->omode,NC_INMEMORY)) {
	/* noop */
#ifdef ENABLE_BYTERANGE
    } else if(file->uri != NULL) {
	status = nc_http_close(file->state);
	nullfree(file->curlurl);
#endif
    } else {
#ifdef USE_PARALLEL
        if (file->use_parallel) {
	    int retval;
	    if((retval = MPI_File_close(&file->fh)) != MPI_SUCCESS)
		    {status = NC_EPARINIT; return status;}
        } else
#endif
        {
	    if(file->fp) fclose(file->fp);
        }
    }
    return status;
}

/*!
  Interpret the magic number found in the header of a netCDF file.
  This function interprets the magic number/string contained in the header of a netCDF file and sets the appropriate NC_FORMATX flags.

  @param[in] magic Pointer to a character array with the magic number block.
  @param[out] model Pointer to an integer to hold the corresponding netCDF type.
  @param[out] version Pointer to an integer to hold the corresponding netCDF version.
  @returns NC_NOERR if a legitimate file type found
  @returns NC_ENOTNC otherwise

\internal
\ingroup datasets

*/
static int
NC_interpret_magic_number(char* magic, NCmodel* model)
{
    int status = NC_NOERR;
    /* Look at the magic number */
#ifdef USE_NETCDF4
    if (strlen(UDF0_magic_number) && !strncmp(UDF0_magic_number, magic,
                                              strlen(UDF0_magic_number)))
    {
	model->impl = NC_FORMATX_UDF0;
	model->format = NC_FORMAT_NETCDF4;
	goto done;
    }
    if (strlen(UDF1_magic_number) && !strncmp(UDF1_magic_number, magic,
                                              strlen(UDF1_magic_number)))
    {
	model->impl = NC_FORMATX_UDF1;
	model->format = NC_FORMAT_NETCDF4;
	goto done;
    }
#endif /* USE_NETCDF4 */

    /* Use the complete magic number string for HDF5 */
    if(memcmp(magic,HDF5_SIGNATURE,sizeof(HDF5_SIGNATURE))==0) {
	model->impl = NC_FORMATX_NC4;
	model->format = NC_FORMAT_NETCDF4;
	goto done;
    }
    if(magic[0] == '\016' && magic[1] == '\003'
              && magic[2] == '\023' && magic[3] == '\001') {
	model->impl = NC_FORMATX_NC_HDF4;
	model->format = NC_FORMAT_NETCDF4;
	goto done;
    }
    if(magic[0] == 'C' && magic[1] == 'D' && magic[2] == 'F') {
        if(magic[3] == '\001') {
	    model->impl = NC_FORMATX_NC3;
	    model->format = NC_FORMAT_CLASSIC;
	    goto done;
	}
        if(magic[3] == '\002') {
	    model->impl = NC_FORMATX_NC3;
	    model->format = NC_FORMAT_64BIT_OFFSET;
	    goto done;
        }
        if(magic[3] == '\005') {
	  model->impl = NC_FORMATX_NC3;
	  model->format = NC_FORMAT_64BIT_DATA;
	  goto done;
	}
     }
     /* No match  */
     status = NC_ENOTNC;
     goto done;

done:
     return check(status);
}

#ifdef DEBUG
static void
printmagic(const char* tag, char* magic, struct MagicFile* f)
{
    int i;
    fprintf(stderr,"%s: ispar=%d magic=",tag,f->use_parallel);
    for(i=0;i<MAGIC_NUMBER_LEN;i++) {
        unsigned int c = (unsigned int)magic[i];
	c = c & 0x000000FF;
	if(c == '\n')
	    fprintf(stderr," 0x%0x/'\\n'",c);
	else if(c == '\r')
	    fprintf(stderr," 0x%0x/'\\r'",c);
	else if(c < ' ')
	    fprintf(stderr," 0x%0x/'?'",c);
	else
	    fprintf(stderr," 0x%0x/'%c'",c,c);
    }
    fprintf(stderr,"\n");
    fflush(stderr);
}

static void
printlist(NClist* list, const char* tag)
{
    int i;
    fprintf(stderr,"%s:",tag);
    for(i=0;i<nclistlength(list);i++)
        fprintf(stderr," %s",(char*)nclistget(list,i));
    fprintf(stderr,"\n");
    dbgflush();
}


#endif
