/**
 * @file
 *
 * Infer as much as possible from the omode + path.
 * Possibly rewrite the path.
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
#include "ncwinpath.h"
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
    NCmodel* model;
    fileoffset_t filelen;
    int use_parallel;
    void* parameters; /* !NULL if inmemory && !diskless */
    FILE* fp;
#ifdef USE_PARALLEL
    MPI_File fh;
#endif
#ifdef ENABLE_BYTERANGE
    void* curl; /* avoid need to include curl.h */
    char* curlurl; /* url to use with CURLOPT_SET_URL */
#endif
};

/** @internal Magic number for HDF5 files. To be consistent with
 * H5Fis_hdf5, use the complete HDF5 magic number */
static char HDF5_SIGNATURE[MAGIC_NUMBER_LEN] = "\211HDF\r\n\032\n";

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

#define modelcomplete(model) ((model)->format != 0 && (model)->iosp != 0 && (model)->impl != 0)

enum mfield {MF, MI, MIO, MV};

/* Wrap model field assignment to fail if the
   existing value is not zero and not same as src value
*/
#define conflictset(f,dst,src) do {if((dst) != 0 && (src) != (dst)) {stat=conflictfail(f,(dst),(src)); goto done;} else {(dst) = (src);} } while(0)

/*
Define a table of iosp string values for "mode=".
Includes cases where the impl or format implies the
iosp. Does not includes cases where NC_IOSP_FILE is
the inferred iosp.
*/
static struct IOSPS {
    const char* tag;
    const int iosp; /* NC_IOSP_XXX value */
} iosps[] = {
{"dap2",NC_IOSP_DAP2},
{"dap4",NC_IOSP_DAP4},
{"bytes",NC_IOSP_HTTP},
{NULL,0}
};

/*
Define a table of "mode=" string values.
Note that only cases that can currently
take URLs are included.
*/
static struct FORMATMODES {
    const char* tag;
    const int format; /* NC_FORMAT_XXX value */
    const int impl; /* NC_FORMATX_XXX value */
} formatmodes[] = {
{"dap2",NC_FORMAT_CLASSIC,NC_FORMATX_DAP2},
{"dap4",NC_FORMAT_NETCDF4,NC_FORMATX_DAP4},
{"netcdf-3",NC_FORMAT_CLASSIC,NC_FORMATX_NC3},
{"classic",NC_FORMAT_CLASSIC,NC_FORMATX_NC3},
{"netcdf-4",NC_FORMAT_NETCDF4,NC_FORMATX_NC4},
{"enhanced",NC_FORMAT_NETCDF4,NC_FORMATX_NC4},
{"64bitoffset",NC_FORMAT_64BIT_OFFSET,0},
{"64bitdata",NC_FORMAT_64BIT_DATA,0},
{"cdf5",NC_FORMAT_64BIT_DATA,0}, /*alias*/
#if 0
{"hdf4",NC_FORMAT_HDF4,NC_FORMATX_NC4},
#endif
{NULL,0,0},
};

/* Define the legal singleton mode tags */
static const char* modesingles[] = {
    "dap2", "dap4", "bytes", "zarr", NULL,
};

/* Map IOSP to readability to get magic number */
static struct IospRead {
    int iosp;
    int readable;
} readable[] = {
{NC_IOSP_FILE,1},
{NC_IOSP_MEMORY,1},
{NC_IOSP_UDF,0},
{NC_IOSP_HTTP,1},
{0,0},
};

/* Define the known URL protocols and their interpretation */
static struct NCPROTOCOLLIST {
    const char* protocol;
    const char* substitute;
    const char* mode;
} ncprotolist[] = {
    {"http",NULL,NULL},
    {"https",NULL,NULL},
    {"file",NULL,NULL},
    {"dods","http","dap2"},
    {"dap4","http","dap4"},
    {NULL,NULL,NULL} /* Terminate search */
};

/* Forward */
static int NC_omodeinfer(int omode, NCmodel*);
static int NC_implinfer(int useparallel, NCmodel* model);
static int NC_dapinfer(NClist*, NCmodel* model);
static int check_file_type(const char *path, int flags, int use_parallel, void *parameters, NCmodel* model, NCURI* uri);
static int processuri(const char* path, NCURI** urip, char** newpathp, NClist* modeargs);
static int extractiosp(NClist* modeargs, int mode, NCmodel* model);

static int openmagic(struct MagicFile* file);
static int readmagic(struct MagicFile* file, long pos, char* magic);
static int closemagic(struct MagicFile* file);
static int NC_interpret_magic_number(char* magic, NCmodel* model);
#ifdef DEBUG
static void printmagic(const char* tag, char* magic,struct MagicFile*);
#endif
static int isreadable(int iosp);

/* Report a conflicting model field assignment;
   see the conflictset macro above */
static int
conflictfail(enum mfield f, int dst, int src)
{
    const char* sf = NULL;
    switch (f) {
    case MF: sf = "format"; break;
    case MI: sf = "impl"; break;
    case MIO: sf = "iosp"; break;
    case MV: sf = "version"; break;
    default: sf = "?"; break;
    }
    nclog(NCLOGERR,"Model inference conflict: field=%s dst=%d src=%d", sf,dst,src);
    return NC_EINVAL;
}

/* Parse a mode string at the commas and convert to envv form */
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

/* Given a mode= argument, and the mode flags,
   infer the iosp part of the model */
static int
extractiosp(NClist* modeargs, int cmode, NCmodel* model)
{
    int stat = NC_NOERR;
    struct IOSPS* io = iosps;

    assert(model->iosp == 0);
    for(;io->tag;io++) {
	int i;
	for(i=0;i<nclistlength(modeargs);i++) {
	    const char* p = nclistget(modeargs,i);
	    if(strcmp(p,io->tag)==0) {
		conflictset(MIO,model->iosp,io->iosp);
		goto done;
	    }
	}
    }
done:
    if(model->iosp == 0)
	model->iosp = (fIsSet(cmode,NC_INMEMORY) ? NC_IOSP_MEMORY:NC_IOSP_FILE);
    return stat;
}

/* Given a mode= argument, fill in the matching part of the model; except IOSP */
static int
processmodearg(const char* arg, NCmodel* model)
{
    int stat = NC_NOERR;
    struct FORMATMODES* format = formatmodes;
    for(;format->tag;format++) {
	if(strcmp(format->tag,arg)==0) {
	    conflictset(MF,model->format,format->format);
	    conflictset(MI,model->impl,format->impl);
	}
    }
done:
    return check(stat);
}

/* Search singleton list */
static int
issingleton(const char* tag)
{
    const char** p;
    for(p=modesingles;*p;p++) {
	if(strcmp(*p,tag)==0) return 1;
    }
    return 0;
}

/* If we have a url, see if we can determine DAP */
static int
NC_dapinfer(NClist* modeargs, NCmodel* model)
{
    int stat = NC_NOERR;
    int i;

    /* 1. search modeargs for indicators */
    for(i=0;i<nclistlength(modeargs);i++) {
	const char* arg = nclistget(modeargs,i);
	if(strcasecmp(arg,"bytes")==0
	   || strcasecmp(arg,"zarr")==0) {
	    /* Ok, we know this is not DAP, so give up */
	    return stat;
	}
	if(strcasecmp(arg,"dap2")==0) {
	    model->format = NC_FORMAT_NC3;
	    model->iosp = NC_IOSP_DAP2;
	    model->impl = NC_FORMATX_DAP2;
	} else if(strcasecmp(arg,"dap4")==0) {
	    model->format = NC_FORMAT_NETCDF4;
	    model->iosp = NC_IOSP_DAP4;
	    model->impl = NC_FORMATX_DAP4;
	}
    }
    /* Ok, we have a URL, but no tags to tell us what it is, so assume DAP2 */
    if(model->impl == 0) {
	model->format = NC_FORMAT_NC3;
	model->iosp = NC_IOSP_DAP2;
	model->impl = NC_FORMATX_DAP2;
    }
    return stat;
}

/*
Infer from the mode
only call if iscreate or file is not easily readable.
*/
static int
NC_omodeinfer(int cmode, NCmodel* model)
{
    int stat = NC_NOERR;

    /* If no format flags are set, then use default */
    if(!fIsSet(cmode,NC_FORMAT_ALL))
	conflictset(MF,model->format,nc_get_default_format());

    /* Process the cmode; may override some already set flags */
    if(fIsSet(cmode,NC_64BIT_OFFSET)) {
	conflictset(MF,model->format,NC_FORMAT_64BIT_OFFSET);
    }
    if(fIsSet(cmode,NC_64BIT_DATA)) {
	conflictset(MF,model->format,NC_FORMAT_64BIT_DATA);
    }
    if(fIsSet(cmode,NC_NETCDF4)) {
	conflictset(MF,model->format,NC_FORMAT_NETCDF4);
    }
    if(fIsSet(cmode,(NC_UDF0|NC_UDF1))) {
	conflictset(MF,model->format,NC_FORMAT_NETCDF4);
	/* For user formats, we must back out some previous decisions */
	model->iosp = NC_IOSP_UDF; /* Do not know anything about this */
        if(fIsSet(cmode,NC_UDF0)) {
	    conflictset(MI,model->impl,NC_FORMATX_UDF0);
	} else {
	    conflictset(MI,model->impl,NC_FORMATX_UDF1);
	}
    }
    /* Ignore following flags for now */
#if 0 /* keep lgtm happy */
    if(fIsSet(cmode,NC_CLASSIC_MODEL)) {}
    if(fIsSet(cmode,NC_DISKLESS)) {}
#endif

done:
    return check(stat);
}

/* Infer the implementation/dispatcher from format*/
static int
NC_implinfer(int useparallel, NCmodel* model)
{
    int stat = NC_NOERR;

    /* If we do not have a format, then use default format */
    if(model->format == 0)
	conflictset(MF,model->format,nc_get_default_format());

    /* Try to infer impl based on format; may modify mode flags */
    if(model->impl == 0) {
        switch (model->format) {
        case NC_FORMAT_NETCDF4:
             conflictset(MI,model->impl,NC_FORMATX_NC4);
             break;
        case NC_FORMAT_NETCDF4_CLASSIC:
             conflictset(MI,model->impl,NC_FORMATX_NC4);
             break;
        case NC_FORMAT_CDF5:
             conflictset(MI,model->impl,NC_FORMATX_NC3);
             break;
        case NC_FORMAT_64BIT_OFFSET:
             conflictset(MI,model->impl,NC_FORMATX_NC3);
             break;
        case NC_FORMAT_CLASSIC:
             conflictset(MI,model->impl,NC_FORMATX_NC3);
             break;
        default: break;
        }
        /* default dispatcher if above did not infer an implementation */
        if (model->impl == 0)
            conflictset(MI,model->impl,NC_FORMATX_NC3); /* Final choice */
        /* Check for using PNETCDF */
        if (model->impl== NC_FORMATX_NC3
		&& useparallel
		&& model->iosp == NC_IOSP_FILE)
            model->impl = NC_FORMATX_PNETCDF; /* Use this instead */
    }

    assert(model->impl != 0);
done:
    return check(stat);
}

static int
processuri(const char* path, NCURI** urip, char** newpathp, NClist* modeargs)
{
    int stat = NC_NOERR;
    int found = 0;
    const char** fragp = NULL;
    struct NCPROTOCOLLIST* protolist;
    NCURI* uri = NULL;
    size_t pathlen = strlen(path);

    if(path == NULL || pathlen == 0) {stat = NC_EURL; goto done;}

    /* Defaults */
    if(newpathp) *newpathp = NULL;
    if(urip) *urip = NULL;

    if(ncuriparse(path,&uri) != NCU_OK) goto done; /* not url */

    /* Look up the protocol */
    for(found=0,protolist=ncprotolist;protolist->protocol;protolist++) {
        if(strcmp(uri->protocol,protolist->protocol) == 0) {
	    found = 1;
	    break;
	}
    }
    if(!found)
	{stat = NC_EINVAL; goto done;} /* unrecognized URL form */

    /* process the corresponding mode arg */
    if(protolist->mode != NULL)
	nclistpush(modeargs,strdup(protolist->mode));

    /* Substitute the protocol in any case */
    if(protolist->substitute) ncurisetprotocol(uri,protolist->substitute);

    /* Iterate over the url fragment parameters */
    for(fragp=ncurifragmentparams(uri);fragp && *fragp;fragp+=2) {
	const char* name = fragp[0];
	const char* value = fragp[1];
	if(strcmp(name,"protocol")==0) {
	    nclistpush(modeargs,strdup(value));
	} else
	if(strcmp(name,"mode")==0) {
	    if((stat = parseurlmode(value,modeargs))) goto done;
	} else
	if(issingleton(name) && (value == NULL || strlen(value)==0)) {
	    nclistpush(modeargs,strdup(name));
        } /*else ignore*/
    }

    /* At this point modeargs should contain all mode args from the URL */

    /* Rebuild the path (including fragment)*/
    if(newpathp)
        *newpathp = ncuribuild(uri,NULL,NULL,NCURIALL);
    if(urip) {
	*urip = uri;
	uri = NULL;
    }
done:
    if(uri != NULL) ncurifree(uri);
    return check(stat);
}

/**************************************************/
/*
   Infer model for this dataset using some
   combination of cmode, path, and reading the dataset.

   The precedence order is:
   1. file contents -- highest precedence
   2. path
   2. isurl -- check for DAP
   3. mode
   4. default format -- lowest precedence

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
    int stat = NC_NOERR;
    char* newpath = NULL;
    NCURI* uri = NULL;
    int omode = *omodep;
    int isuri = 0;
    NClist* modeargs = nclistnew();

    if((stat = processuri(path, &uri, &newpath, modeargs))) goto done;
    isuri = (uri != NULL);

    /* Phase 1: compute the IOSP */
    if((stat = extractiosp(modeargs,omode,model))) goto done;
    assert(model->iosp != 0);

    /* Phase 2: Process the non-iosp mode arguments */
    if(!modelcomplete(model) && isuri) {
	    int i;
	    for(i=0;i<nclistlength(modeargs);i++) {
		const char* arg = nclistget(modeargs,i);
		if((stat=processmodearg(arg,model))) goto done;
	    }
    }

    /* Phase 3: See if we can infer DAP */
    if(!modelcomplete(model) && isuri) {
            if((stat = NC_dapinfer(modeargs,model))) goto done;
    }

    /* Phase 4: mode inference */
    if(!modelcomplete(model)) {
        if((stat = NC_omodeinfer(omode,model))) goto done;
    }

    /* Phase 5: Infer from file content, if possible;
       this has highest precedence, so it may override
       previous decisions.
    */
    if(!iscreate && isreadable(model->iosp)) {
	/* Ok, we need to try to read the file */
	if((stat = check_file_type(path, omode, useparallel, params, model, uri))) goto done;
    }

    /* Phase 6: Infer impl from format */
    if(!modelcomplete(model)) {
        if((stat = NC_implinfer(useparallel, model))) goto done;
    }

    assert(modelcomplete(model));

    /* Force flag consistency */
    switch (model->impl) {
    case NC_FORMATX_NC4:
    case NC_FORMATX_NC_HDF4:
    case NC_FORMATX_DAP4:
    case NC_FORMATX_UDF0:
    case NC_FORMATX_UDF1:
	omode |= NC_NETCDF4;
	if(model->format == NC_FORMAT_NETCDF4_CLASSIC)
	    omode |= NC_CLASSIC_MODEL;
	break;
    case NC_FORMATX_DAP2:
	omode &= ~(NC_NETCDF4|NC_64BIT_OFFSET|NC_64BIT_DATA);
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
    default:
	{stat = NC_ENOTNC; goto done;}
    }

done:
    if(uri) ncurifree(uri);
    nclistfreeall(modeargs);
    if(stat == NC_NOERR && newpathp) {*newpathp = newpath; newpath = NULL;}
    nullfree(newpath);
    *omodep = omode; /* in/out */
    return check(stat);
}

static int
isreadable(int iosp)
{
    struct IospRead* r;
    /* Look up the protocol */
    for(r=readable;r->iosp;r++) {
	if(iosp == r->iosp) return r->readable;
    }
    return 0;
}

/**************************************************/
#if 0
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
#endif

/**************************************************/
/**
 * Provide a hidden interface to allow utilities
 * to check if a given path name is really a url.
 * If not, put null in basenamep, else put basename of the url
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
check_file_type(const char *path, int flags, int use_parallel,
		   void *parameters, NCmodel* model, NCURI* uri)
{
    char magic[NC_MAX_MAGIC_NUMBER_LEN];
    int status = NC_NOERR;
    struct MagicFile magicinfo;

    memset((void*)&magicinfo,0,sizeof(magicinfo));
    magicinfo.path = path; /* do not free */
    magicinfo.uri = uri; /* do not free */
    magicinfo.model = model; /* do not free */
    magicinfo.parameters = parameters; /* do not free */
#ifdef USE_STDIO
    magicinfo.use_parallel = 0;
#else
    magicinfo.use_parallel = use_parallel;
#endif

    if((status = openmagic(&magicinfo))) goto done;

    /* Verify we have a large enough file */
    if(magicinfo.filelen < MAGIC_NUMBER_LEN)
	{status = NC_ENOTNC; goto done;}
    if((status = readmagic(&magicinfo,0L,magic)) != NC_NOERR) {
	status = NC_ENOTNC;
	goto done;
    }

    /* Look at the magic number */
    if(NC_interpret_magic_number(magic,model) == NC_NOERR
	&& model->format != 0) {
        if (model->format == NC_FORMAT_NC3 && use_parallel)
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

    switch (file->model->iosp) {
    case NC_IOSP_MEMORY: {
	/* Get its length */
	NC_memio* meminfo = (NC_memio*)file->parameters;
        assert(meminfo != NULL);
	file->filelen = (long long)meminfo->size;
	} break;
    case NC_IOSP_FILE: {
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
	} break;

#ifdef ENABLE_BYTERANGE
    case NC_IOSP_HTTP: {
	/* Construct a URL minus any fragment */
        file->curlurl = ncuribuild(file->uri,NULL,NULL,NCURISVC);
	/* Open the curl handle */
	if((status=nc_http_open(file->curlurl,&file->curl,&file->filelen))) goto done;
	} break;
#endif

    default: assert(0);
    }

done:
    return check(status);
}

static int
readmagic(struct MagicFile* file, long pos, char* magic)
{
    int status = NC_NOERR;
    memset(magic,0,MAGIC_NUMBER_LEN);
    switch (file->model->iosp) {
    case NC_IOSP_MEMORY: {
	char* mempos;
	NC_memio* meminfo = (NC_memio*)file->parameters;
	if((pos + MAGIC_NUMBER_LEN) > meminfo->size)
	    {status = NC_EINMEMORY; goto done;}
	mempos = ((char*)meminfo->memory) + pos;
	memcpy((void*)magic,mempos,MAGIC_NUMBER_LEN);
#ifdef DEBUG
	printmagic("XXX: readmagic",magic,file);
#endif
    } break;

    case NC_IOSP_FILE:
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
	break;

#ifdef ENABLE_BYTERANGE
    case NC_IOSP_HTTP: {
	NCbytes* buf = ncbytesnew();
	fileoffset_t start = (size_t)pos;
	fileoffset_t count = MAGIC_NUMBER_LEN;
	status = nc_http_read(file->curl,file->curlurl,start,count,buf);
	if(status == NC_NOERR) {
	    if(ncbyteslength(buf) != count)
	        status = NC_EINVAL;
	    else
	        memcpy(magic,ncbytescontents(buf),count);
	}
	ncbytesfree(buf);
        } break;
#endif

    default: assert(0);
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
    switch (file->model->iosp) {
    case NC_IOSP_MEMORY:
	break; /* noop */

    case NC_IOSP_FILE:
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
	break;

#ifdef ENABLE_BYTERANGE
     case NC_IOSP_HTTP:
	status = nc_http_close(file->curl);
	nullfree(file->curlurl);
	break;
#endif

    default: assert(0);
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
#endif
