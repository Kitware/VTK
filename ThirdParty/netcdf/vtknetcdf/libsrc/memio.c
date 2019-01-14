/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *	See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
#if defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#include <winbase.h>
#include <io.h>
#define lseek64 lseek
#endif

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifdef _MSC_VER /* Microsoft Compilers */
#include <io.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "ncdispatch.h"
#include "nc3internal.h"

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

#ifndef HAVE_SSIZE_T
typedef int ssize_t;
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* Define the mode flags for create: let umask rule */
#define OPENMODE 0666

#include "ncio.h"
#include "fbits.h"
#include "rnd.h"

/* #define INSTRUMENT 1 */
#if INSTRUMENT /* debugging */
#undef NDEBUG
#include <stdio.h>
#include "instr.h"
#endif

#ifndef MEMIO_MAXBLOCKSIZE
#define MEMIO_MAXBLOCKSIZE 268435456 /* sanity check, about X_SIZE_T_MAX/8 */
#endif

#undef MIN  /* system may define MIN somewhere and complain */
#define MIN(mm,nn) (((mm) < (nn)) ? (mm) : (nn))

#if !defined(NDEBUG) && !defined(X_INT_MAX)
#define  X_INT_MAX 2147483647
#endif

#if 0 /* !defined(NDEBUG) && !defined(X_ALIGN) */
#define  X_ALIGN 4
#else
#undef X_ALIGN
#endif

/* Private data for memio */

typedef struct NCMEMIO {
    int locked; /* => we cannot realloc */
    int persist; /* => save to a file; triggered by NC_WRITE */
    char* memory;
    off_t alloc;
    off_t size;
    off_t pos;
} NCMEMIO;

/* Forward */
static int memio_rel(ncio *const nciop, off_t offset, int rflags);
static int memio_get(ncio *const nciop, off_t offset, size_t extent, int rflags, void **const vpp);
static int memio_move(ncio *const nciop, off_t to, off_t from, size_t nbytes, int rflags);
static int memio_sync(ncio *const nciop);
static int memio_filesize(ncio* nciop, off_t* filesizep);
static int memio_pad_length(ncio* nciop, off_t length);
static int memio_close(ncio* nciop, int);

/* Mnemonic */
#define DOOPEN 1

static long pagesize = 0;

/*! Create a new ncio struct to hold info about the file. */
static int memio_new(const char* path, int ioflags, off_t initialsize, void* memory, ncio** nciopp, NCMEMIO** memiop)
{
    int status = NC_NOERR;
    ncio* nciop = NULL;
    NCMEMIO* memio = NULL;
    off_t minsize = initialsize;
    int inmemory = (fIsSet(ioflags,NC_INMEMORY));

    /* use asserts because this is an internal function */
    assert(memiop != NULL && nciopp != NULL);
    assert(path != NULL || (memory != NULL && initialsize > 0));
    assert(!inmemory || (memory != NULL && initialsize > 0));

    if(pagesize == 0) {
#if defined (_WIN32) || defined(_WIN64)
      SYSTEM_INFO info;
      GetSystemInfo (&info);
      pagesize = info.dwPageSize;
#elif defined HAVE_SYSCONF
        pagesize = sysconf(_SC_PAGE_SIZE);
#elif defined HAVE_GETPAGESIZE
        pagesize = getpagesize();
#else
        pagesize = 4096; /* good guess */
#endif
    }

    /* We need to catch errors.
       sysconf, at least, can return a negative value
       when there is an error. */
    if(pagesize < 0) {
      status = NC_EIO;
      goto fail;
    }

    errno = 0;

    /* Always force the allocated size to be a multiple of pagesize */
    if(initialsize == 0) initialsize = pagesize;
    if((initialsize % pagesize) != 0)
	initialsize += (pagesize - (initialsize % pagesize));

    nciop = (ncio* )calloc(1,sizeof(ncio));
    if(nciop == NULL) {status = NC_ENOMEM; goto fail;}

    nciop->ioflags = ioflags;
    *((int*)&nciop->fd) = -1; /* caller will fix */

    *((ncio_relfunc**)&nciop->rel) = memio_rel;
    *((ncio_getfunc**)&nciop->get) = memio_get;
    *((ncio_movefunc**)&nciop->move) = memio_move;
    *((ncio_syncfunc**)&nciop->sync) = memio_sync;
    *((ncio_filesizefunc**)&nciop->filesize) = memio_filesize;
    *((ncio_pad_lengthfunc**)&nciop->pad_length) = memio_pad_length;
    *((ncio_closefunc**)&nciop->close) = memio_close;

    memio = (NCMEMIO*)calloc(1,sizeof(NCMEMIO));
    if(memio == NULL) {status = NC_ENOMEM; goto fail;}
    *((void* *)&nciop->pvt) = memio;

    *((char**)&nciop->path) = strdup(path);
    if(nciop->path == NULL) {status = NC_ENOMEM; goto fail;}
    memio->alloc = initialsize;
    memio->pos = 0;
    memio->size = minsize;
    memio->memory = NULL;
    memio->persist = fIsSet(ioflags,NC_WRITE);
    if(memiop && memio) *memiop = memio; else free(memio);
    if(nciopp && nciop) *nciopp = nciop;
    else {
        if(nciop->path != NULL) free((char*)nciop->path);
        free(nciop);
    }
    if(inmemory) {
      memio->memory = memory;
    } else {
        /* malloc memory */
        memio->memory = (char*)malloc(memio->alloc);
        if(memio->memory == NULL) {status = NC_ENOMEM; goto fail;}
    }

done:
    return status;

fail:
    if(memio != NULL) free(memio);
    if(nciop != NULL) {
        if(nciop->path != NULL) free((char*)nciop->path);
        free(nciop);
    }
    goto done;
}

/* Create a file, and the ncio struct to go with it.

   path - path of file to create.
   ioflags - flags from nc_create
   initialsz - From the netcdf man page: "The argument
               initialsize sets the initial size of the file at creation time."
   igeto -
   igetsz -
   sizehintp - the size of a page of data for buffered reads and writes.
   parameters - arbitrary data
   nciopp - pointer to a pointer that will get location of newly
   created and inited ncio struct.
   mempp - pointer to pointer to the initial memory read.
*/
int
memio_create(const char* path, int ioflags,
    size_t initialsz,
    off_t igeto, size_t igetsz, size_t* sizehintp,
    void* parameters,
    ncio* *nciopp, void** const mempp)
{
    ncio* nciop;
    int fd;
    int status;
    NCMEMIO* memio = NULL;
    int persist = (ioflags & NC_WRITE?1:0);
    int oflags;

    if(path == NULL ||* path == 0)
        return NC_EINVAL;

    status = memio_new(path, ioflags, initialsz, NULL, &nciop, &memio);
    if(status != NC_NOERR)
        return status;

    if(persist) {
        /* Open the file just tomake sure we can write it if needed */
        oflags = (persist ? O_RDWR : O_RDONLY);
#ifdef O_BINARY
        fSet(oflags, O_BINARY);
#endif
    	oflags |= (O_CREAT|O_TRUNC);
        if(fIsSet(ioflags,NC_NOCLOBBER))
	    oflags |= O_EXCL;
#ifdef vms
        fd = open(path, oflags, 0, "ctx=stm");
#else
        fd  = open(path, oflags, OPENMODE);
#endif
        if(fd < 0) {status = errno; goto unwind_open;}

        (void)close(fd); /* will reopen at nc_close */
    } /*!persist*/

#ifdef DEBUG
fprintf(stderr,"memio_create: initial memory: %lu/%lu\n",(unsigned long)memio->memory,(unsigned long)memio->alloc);
#endif

    fd = nc__pseudofd();
    *((int* )&nciop->fd) = fd;

    fSet(nciop->ioflags, NC_WRITE);

    if(igetsz != 0)
    {
        status = nciop->get(nciop,
                igeto, igetsz,
                RGN_WRITE,
                mempp);
        if(status != NC_NOERR)
            goto unwind_open;
    }

    /* Pick a default sizehint */
    if(sizehintp) *sizehintp = pagesize;

    *nciopp = nciop;
    return NC_NOERR;

unwind_open:
    memio_close(nciop,1);
    return status;
}

/* This function opens the data file.

   path - path of data file.
   ioflags - flags passed into nc_open.
   igeto - looks like this function can do an initial page get, and
   igeto is going to be the offset for that. But it appears to be
   unused
   igetsz - the size in bytes of initial page get (a.k.a. extent). Not
   ever used in the library.
   sizehintp - the size of a page of data for buffered reads and writes.
   parameters - arbitrary data
   nciopp - pointer to pointer that will get address of newly created
   and inited ncio struct.
   mempp - pointer to pointer to the initial memory read.
*/
int
memio_open(const char* path,
    int ioflags,
    off_t igeto, size_t igetsz, size_t* sizehintp,
    void* parameters,
    ncio* *nciopp, void** const mempp)
{
    ncio* nciop = NULL;
    int fd = -1;
    int status = NC_NOERR;
    int persist = (fIsSet(ioflags,NC_WRITE)?1:0);
    int inmemory = (fIsSet(ioflags,NC_INMEMORY));
    int oflags = 0;
    NCMEMIO* memio = NULL;
    size_t sizehint = 0;
    off_t filesize = 0;
    off_t red = 0;
    char* pos = NULL;
    NC_MEM_INFO* meminfo = (NC_MEM_INFO*)parameters;

    if(path == NULL || strlen(path) == 0)
        return NC_EINVAL;

    assert(sizehintp != NULL);
    sizehint = *sizehintp;

    if(inmemory) {
	filesize = meminfo->size;
    } else {
        /* Open the file,and make sure we can write it if needed */
        oflags = (persist ? O_RDWR : O_RDONLY);
#ifdef O_BINARY
        fSet(oflags, O_BINARY);
#endif
        oflags |= O_EXCL;
#ifdef vms
        fd = open(path, oflags, 0, "ctx=stm");
#else
        fd  = open(path, oflags, OPENMODE);
#endif
#ifdef DEBUG
        if(fd < 0) {
            fprintf(stderr,"open failed: file=%s err=",path);
            perror("");
	}
#endif
        if(fd < 0) {status = errno; goto unwind_open;}

        /* get current filesize  = max(|file|,initialize)*/
        filesize = lseek(fd,0,SEEK_END);
        if(filesize < 0) {status = errno; goto unwind_open;}
        /* move pointer back to beginning of file */
        (void)lseek(fd,0,SEEK_SET);
        if(filesize < (off_t)sizehint)
            filesize = (off_t)sizehint;
    }

    if(inmemory)
        status = memio_new(path, ioflags, filesize, meminfo->memory, &nciop, &memio);
    else
        status = memio_new(path, ioflags, filesize, NULL, &nciop, &memio);
    if(status != NC_NOERR) {
	if(fd >= 0)
	    close(fd);
      return status;
    }

#ifdef DEBUG
fprintf(stderr,"memio_open: initial memory: %lu/%lu\n",(unsigned long)memio->memory,(unsigned long)memio->alloc);
#endif

    if(!inmemory) {
        /* Read the file into the memio memory */
        /* We need to do multiple reads because there is no
           guarantee that the amount read will be the full amount */
        red = memio->size;
        pos = memio->memory;
        while(red > 0) {
            ssize_t count = read(fd, pos, red);
            if(count < 0) {status = errno; goto unwind_open;}
            if(count == 0) {status = NC_ENOTNC; goto unwind_open;}
            red -= count;
            pos += count;
        }
        (void)close(fd);
    }

    /* Use half the filesize as the blocksize ; why? */
    sizehint = filesize/2;

    /* sizehint must be multiple of 8 */
    sizehint = (sizehint / 8) * 8;
    if(sizehint < 8) sizehint = 8;

    fd = nc__pseudofd();
    *((int* )&nciop->fd) = fd;

    if(igetsz != 0)
    {
        status = nciop->get(nciop,
                igeto, igetsz,
                0,
                mempp);
        if(status != NC_NOERR)
            goto unwind_open;
    }

    if(sizehintp) *sizehintp = sizehint;
    if(nciopp) *nciopp = nciop; else {ncio_close(nciop,0);}
    return NC_NOERR;

unwind_open:
    if(fd >= 0)
      close(fd);
    memio_close(nciop,0);
    return status;
}

/*
 *  Get file size in bytes.
 */
static int
memio_filesize(ncio* nciop, off_t* filesizep)
{
    NCMEMIO* memio;
    if(nciop == NULL || nciop->pvt == NULL) return NC_EINVAL;
    memio = (NCMEMIO*)nciop->pvt;
    if(filesizep != NULL) *filesizep = memio->size;
    return NC_NOERR;
}

/*
 *  Sync any changes to disk, then truncate or extend file so its size
 *  is length.  This is only intended to be called before close, if the
 *  file is open for writing and the actual size does not match the
 *  calculated size, perhaps as the result of having been previously
 *  written in NOFILL mode.
 */
static int
memio_pad_length(ncio* nciop, off_t length)
{
    NCMEMIO* memio;
    if(nciop == NULL || nciop->pvt == NULL) return NC_EINVAL;
    memio = (NCMEMIO*)nciop->pvt;

    if(!fIsSet(nciop->ioflags, NC_WRITE))
        return EPERM; /* attempt to write readonly file*/

    if(memio->locked > 0)
	return NC_EDISKLESS;

    if(length > memio->alloc) {
        /* Realloc the allocated memory to a multiple of the pagesize*/
	off_t newsize = length;
	void* newmem = NULL;
	/* Round to a multiple of pagesize */
	if((newsize % pagesize) != 0)
	    newsize += (pagesize - (newsize % pagesize));

        newmem = (char*)realloc(memio->memory,newsize);
        if(newmem == NULL) return NC_ENOMEM;

	/* zero out the extra memory */
        memset((void*)((char*)newmem+memio->alloc),0,(newsize - memio->alloc));

#ifdef DEBUG
fprintf(stderr,"realloc: %lu/%lu -> %lu/%lu\n",
(unsigned long)memio->memory,(unsigned long)memio->alloc,
(unsigned long)newmem,(unsigned long)newsize);
#endif
	memio->memory = newmem;
	memio->alloc = newsize;
    }
    memio->size = length;
    return NC_NOERR;
}

/*! Write out any dirty buffers to disk.

  Write out any dirty buffers to disk and ensure that next read will get data from disk. Sync any changes, then close the open file associated with the ncio struct, and free its memory.

  @param[in] nciop pointer to ncio to close.
  @param[in] doUnlink if true, unlink file
  @return NC_NOERR on success, error code on failure.
*/

static int
memio_close(ncio* nciop, int doUnlink)
{
    int status = NC_NOERR;
    NCMEMIO* memio ;
    int fd = -1;
    int inmemory = 0;

    if(nciop == NULL || nciop->pvt == NULL) return NC_NOERR;

    inmemory = (fIsSet(nciop->ioflags,NC_INMEMORY));
    memio = (NCMEMIO*)nciop->pvt;
    assert(memio != NULL);

    /* See if the user wants the contents persisted to a file */
    if(!inmemory && memio->persist) {
        /* Try to open the file for writing */
	int oflags = O_WRONLY|O_CREAT|O_TRUNC;
#ifdef O_BINARY
        fSet(oflags, O_BINARY);
#endif
	fd = open(nciop->path, oflags, OPENMODE);
	if(fd >= 0) {
	    /* We need to do multiple writes because there is no
               guarantee that the amount written will be the full amount */
	    off_t written = memio->size;
	    char* pos = memio->memory;
	    while(written > 0) {
	        ssize_t count = write(fd, pos, written);
	        if(count < 0)
	            {status = errno; goto done;}
	        if(count == 0)
	            {status = NC_ENOTNC; goto done;}
		written -= count;
		pos += count;
	    }
	} else
	    status = errno;
     }

done:
    if(!inmemory && memio->memory != NULL)
	free(memio->memory);
    /* do cleanup  */
    if(fd >= 0) (void)close(fd);
    if(memio != NULL) free(memio);
    if(nciop->path != NULL) free((char*)nciop->path);
    free(nciop);
    return status;
}

static int
guarantee(ncio* nciop, off_t endpoint)
{
    NCMEMIO* memio = (NCMEMIO*)nciop->pvt;
    if(endpoint > memio->alloc) {
	/* extend the allocated memory and size */
	int status = memio_pad_length(nciop,endpoint);
	if(status != NC_NOERR) return status;
    }
    if(memio->size < endpoint)
	memio->size = endpoint;
    return NC_NOERR;
}

/*
 * Request that the region (offset, extent)
 * be made available through *vpp.
 */
static int
memio_get(ncio* const nciop, off_t offset, size_t extent, int rflags, void** const vpp)
{
    int status = NC_NOERR;
    NCMEMIO* memio;
    if(nciop == NULL || nciop->pvt == NULL) return NC_EINVAL;
    memio = (NCMEMIO*)nciop->pvt;
    status = guarantee(nciop, offset+extent);
    memio->locked++;
    if(status != NC_NOERR) return status;
    if(vpp) *vpp = memio->memory+offset;
    return NC_NOERR;
}

/*
 * Like memmove(), safely move possibly overlapping data.
 */
static int
memio_move(ncio* const nciop, off_t to, off_t from, size_t nbytes, int ignored)
{
    int status = NC_NOERR;
    NCMEMIO* memio;

    if(nciop == NULL || nciop->pvt == NULL) return NC_EINVAL;
    memio = (NCMEMIO*)nciop->pvt;
    if(from < to) {
       /* extend if "to" is not currently allocated */
       status = guarantee(nciop,to+nbytes);
       if(status != NC_NOERR) return status;
    }
    /* check for overlap */
    if((to + nbytes) > from || (from + nbytes) > to) {
	/* Ranges overlap */
#ifdef HAVE_MEMMOVE
        memmove((void*)(memio->memory+to),(void*)(memio->memory+from),nbytes);
#else
        off_t overlap;
	off_t nbytes1;
        if((from + nbytes) > to) {
	    overlap = ((from + nbytes) - to); /* # bytes of overlap */
	    nbytes1 = (nbytes - overlap); /* # bytes of non-overlap */
	    /* move the non-overlapping part */
            memcpy((void*)(memio->memory+(to+overlap)),
                   (void*)(memio->memory+(from+overlap)),
		   nbytes1);
	    /* move the overlapping part */
	    memcpy((void*)(memio->memory+to),
                   (void*)(memio->memory+from),
		   overlap);
	} else { /*((to + nbytes) > from) */
	    overlap = ((to + nbytes) - from); /* # bytes of overlap */
	    nbytes1 = (nbytes - overlap); /* # bytes of non-overlap */
	    /* move the non-overlapping part */
            memcpy((void*)(memio->memory+to),
                   (void*)(memio->memory+from),
		   nbytes1);
	    /* move the overlapping part */
	    memcpy((void*)(memio->memory+(to+nbytes1)),
                   (void*)(memio->memory+(from+nbytes1)),
		   overlap);
	}
#endif
    } else {/* no overlap */
	memcpy((void*)(memio->memory+to),(void*)(memio->memory+from),nbytes);
    }
    return status;
}

static int
memio_rel(ncio* const nciop, off_t offset, int rflags)
{
    NCMEMIO* memio;
    if(nciop == NULL || nciop->pvt == NULL) return NC_EINVAL;
    memio = (NCMEMIO*)nciop->pvt;
    memio->locked--;
    return NC_NOERR; /* do nothing */
}

/*
 * Write out any dirty buffers to disk and
 * ensure that next read will get data from disk.
 */
static int
memio_sync(ncio* const nciop)
{
    return NC_NOERR; /* do nothing */
}
