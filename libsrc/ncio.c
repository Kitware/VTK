/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "netcdf.h"
#include "ncio.h"
#include "fbits.h"

/* With the advent of diskless io, we need to provide
   for multiple ncio packages at the same time,
   so we have multiple versions of ncio_create.
*/

/* Define known ncio packages */
extern int posixio_create(const char*,int,size_t,off_t,size_t,size_t*,void*,ncio**,void** const);
extern int posixio_open(const char*,int,off_t,size_t,size_t*,void*,ncio**,void** const);

extern int stdio_create(const char*,int,size_t,off_t,size_t,size_t*,void*,ncio**,void** const);
extern int stdio_open(const char*,int,off_t,size_t,size_t*,void*,ncio**,void** const);

#ifdef USE_FFIO
extern int ffio_create(const char*,int,size_t,off_t,size_t,size_t*,void*,ncio**,void** const);
extern int ffio_open(const char*,int,off_t,size_t,size_t*,void*,ncio**,void** const);
#endif

#ifdef USE_DISKLESS
#  ifdef USE_MMAP
     extern int mmapio_create(const char*,int,size_t,off_t,size_t,size_t*,void*,ncio**,void** const);
     extern int mmapio_open(const char*,int,off_t,size_t,size_t*,void*,ncio**,void** const);
#  endif
     extern int memio_create(const char*,int,size_t,off_t,size_t,size_t*,void*,ncio**,void** const);
     extern int memio_open(const char*,int,off_t,size_t,size_t*,void*,ncio**,void** const);
#endif

int
ncio_create(const char *path, int ioflags, size_t initialsz,
                       off_t igeto, size_t igetsz, size_t *sizehintp,
		       void* parameters,
                       ncio** iopp, void** const mempp)
{
#ifdef USE_DISKLESS
    if(fIsSet(ioflags,NC_DISKLESS)) {
#  ifdef USE_MMAP
      if(fIsSet(ioflags,NC_MMAP))
        return mmapio_create(path,ioflags,initialsz,igeto,igetsz,sizehintp,parameters,iopp,mempp);
      else
#  endif /*USE_MMAP*/
        return memio_create(path,ioflags,initialsz,igeto,igetsz,sizehintp,parameters,iopp,mempp);
    }
#endif

#ifdef USE_STDIO
    return stdio_create(path,ioflags,initialsz,igeto,igetsz,sizehintp,parameters,iopp,mempp);
#elif defined(USE_FFIO)
    return ffio_create(path,ioflags,initialsz,igeto,igetsz,sizehintp,parameters,iopp,mempp);
#else
    return posixio_create(path,ioflags,initialsz,igeto,igetsz,sizehintp,parameters,iopp,mempp);
#endif
}

int
ncio_open(const char *path, int ioflags,
                     off_t igeto, size_t igetsz, size_t *sizehintp,
		     void* parameters,
                     ncio** iopp, void** const mempp)
{
    /* Diskless open has the following constraints:
       1. file must be classic version 1 or 2
     */
#ifdef USE_DISKLESS
    if(fIsSet(ioflags,NC_DISKLESS)) {
#  ifdef USE_MMAP
      if(fIsSet(ioflags,NC_MMAP))
        return mmapio_open(path,ioflags,igeto,igetsz,sizehintp,parameters,iopp,mempp);
      else
#  endif /*USE_MMAP*/
        return memio_open(path,ioflags,igeto,igetsz,sizehintp,parameters,iopp,mempp);
    }
#endif
#ifdef USE_STDIO
    return stdio_open(path,ioflags,igeto,igetsz,sizehintp,parameters,iopp,mempp);
#elif defined(USE_FFIO)
    return ffio_open(path,ioflags,igeto,igetsz,sizehintp,parameters,iopp,mempp);
#else
    return posixio_open(path,ioflags,igeto,igetsz,sizehintp,parameters,iopp,mempp);
#endif
}

/**************************************************/
/* wrapper functions for the ncio dispatch table */

int
ncio_rel(ncio* const nciop, off_t offset, int rflags)
{
    return nciop->rel(nciop,offset,rflags);
}

int
ncio_get(ncio* const nciop, off_t offset, size_t extent,
			int rflags, void **const vpp)
{
    return nciop->get(nciop,offset,extent,rflags,vpp);
}

int
ncio_move(ncio* const nciop, off_t to, off_t from, size_t nbytes, int rflags)
{
    return nciop->move(nciop,to,from,nbytes,rflags);
}

int
ncio_sync(ncio* const nciop)
{
    return nciop->sync(nciop);
}

int
ncio_filesize(ncio* const nciop, off_t *filesizep)
{
    return nciop->filesize(nciop,filesizep);
}

int
ncio_pad_length(ncio* const nciop, off_t length)
{
    return nciop->pad_length(nciop,length);
}

int
ncio_close(ncio* const nciop, int doUnlink)
{
    /* close and release all resources associated
       with nciop, including nciop
    */
    int status = nciop->close(nciop,doUnlink);
    return status;
}
