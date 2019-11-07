/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *	See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*
Implement the ncstdio.h interface using unix stdio.h
*/

/* Forward */
static int NCFile_read(ncstdio*,void*,const size_t,size_t*);
static int NCFile_write(ncstdio*,const void*,const size_t,size_t*);
static int NCFile_free(ncstdio*);
static int NCFile_close(ncstdio*,int);
static int NCFile_flush(ncstdio*)
static int NCFile_seek(ncstdio*,off_t);
static int NCFile_sync(ncstdio*,off_t);
static int NCFile_uid(ncstdio*,int*);

/* Define the stdio.h base operators */


struct NCFile_ops NCFile_ops = {
NCFile_read,
NCFile_write,
NCFile_free,
NCFile_close,
NCFile_flush,
NCFile_seek,
NCFile_sync,
NCFile_uid
};

/* In order to implement the close with delete, we
   need the file path.
*/
struct ncFileState {
    char* path;
    File* file;
};

static struct ncFileState
getState(ncstdio* iop)
{
    if(iop != NULL) {
	if(iop->state != NULL) {
	    return (struct ncFileState*)iop->state;
	}	
    }
    return NULL;
}

int
ncFile_create(const char *path, int ioflags, ncstdio** filepp)
{
    ncstdio* filep;
    File* f;
    struct ncFileState* state;

    f = fopen(path,"w+");
    if(f == NULL)
	return errno;
    filep = (ncstdio*)calloc(sizeof(ncstdio),1);
    if(filep == NULL) {fclose(f); return NC_ENOMEM;}
    state = (ncstdio*)calloc(sizeof(ncFileState),1);
    if(state == NULL) {fclose(f); free(filep); return NC_ENOMEM;}
    filep->ops = &NCFILE_ops;
    filep->ioflags = ioflags;
    filep->state = (void*)state;
	state->path = strdup(path);
	state->file = f;
    if(filepp) *filepp = filep;
    return NC_NOERR;
}

int
ncFile_open(const char *path, int ioflags, ncstdio** filepp)
{
    ncstdio* filep;
    File* f;

    if(fIsSet(ioflags,NC_NOCLOBBER))
        f = fopen(path,"r");
    else
        f = fopen(path,"w+");
    if(f == NULL)
	return errno;

    filep = (ncstdio*)calloc(sizeof(ncstdio),1);
    if(filep == NULL) {fclose(f); return NC_ENOMEM;}
    state = (ncstdio*)calloc(sizeof(ncFileState),1);
    if(state == NULL) {fclose(f); free(filep); return NC_ENOMEM;}
    filep->ops = &NCFILE_ops;
    filep->ioflags = ioflags;
    filep->state = (void*)state;
	state->path = strdup(path);
	state->file = f;
    if(filepp) *filepp = filep;
    return NC_NOERR;
}

static int 
NCFile_close(ncstdio* filep, int delfile)
{
    struct ncFileState* state;
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL || state->file == NULL) return NC_NOERR;
    fclose(state->file);
    state->file = NULL;
    if(delfile)
	unlink(state->path);
    return NC_NOERR;          
}

static int 
NCFile_free(ncstdio* filep)
{
    struct ncFileState* state;
    if(filep == NULL) return NC_NOERR;
    state = (struct ncFileState*)filep->state;
    if(state != NULL) {
	if(state->file != NULL) return NC_EINVAL;
	if(state->path != NULL)
	    free(state->path);
	free(state);
    }
    free(filep);
    return NC_NOERR;          
}

static int
NCFile_flush(ncstdio* filep);
{
    File* state;
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL) return NC_EINVAL;
    if(state->file == NULL) return NC_EINVAL;
    fflush(state->file);	
    return NC_NOERR;
}

static int
NCFile_sync(ncstdio* filep);
{
    File* state;
#ifdef USE_FSYNC
    int fd;
#endif
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL) return NC_EINVAL;
    if(state->file == NULL) return NC_EINVAL;
#ifdef HAVE_FSYNC
#ifdef USE_FSYNC
    fd = fileno(state->file);
#ifndef _WIN32
    fsync(fd);
#else
    _commit(fd);
#endif	/* _WIN32 */
#endif
#endif
    return NC_NOERR;
}

static int
NCFile_seek(ncstdio* filep, off_t pos);
{
    struct ncFileState* state;
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL) return NC_EINVAL;
    if(state->file == NULL) return NC_EINVAL;
    if(!fseek(state->file,pos)) return (errno > 0 ?errno : EINVAL);
    return NC_NOERR;
}

static int
NCFile_read(ncstdio* filep, void* memory, const size_t size, size_t* actualp);
{
    struct ncFileState* state;
    size_t actual;    
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL || state->file == NULL) return NC_EINVAL;
    actual = fread(memory,1,size,state->file);
    if(actualp) *actualp = actual;    
    return (actual < size ? NC_EIO : NC_NOERR);
}

static int
NCFile_write(ncstdio* filep, const void* memory, const size_t size, size_t* actual);
{
    struct ncFileState* state;
    size_t actual;    
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL || state->file == NULL) return NC_EINVAL;
    actual = fwrite(memory,1,size,state->file);
    if(actualp) *actualp = actual;    
    return (actual < size ? NC_EIO : NC_NOERR);
}

static int 
NCFile_uid(ncstdio* filep, int* idp)
{
    struct ncFileState* state;
    if(filep == NULL) return NC_EINVAL;
    state = (struct ncFileState*)filep->state;
    if(state == NULL || state->file == NULL) return NC_EINVAL;
    if(idp) *idp = fileno(state->file);
    return NC_NOERR;
}

