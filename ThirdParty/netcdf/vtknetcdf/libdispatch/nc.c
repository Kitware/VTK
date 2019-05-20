/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(LOCKNUMREC) /* && _CRAYMPP */
#  include <mpp/shmem.h>
#  include <intrinsics.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "nc.h"
#include "ncdispatch.h"

int ncdebug = 0;

/* This is the default create format for nc_create and nc__create. */
static int default_create_format = NC_FORMAT_CLASSIC;

/* These have to do with version numbers. */
#define MAGIC_NUM_LEN 4
#define VER_CLASSIC 1
#define VER_64BIT_OFFSET 2
#define VER_HDF5 3

int
NC_check_id(int ncid, NC** ncpp)
{
    NC* nc = find_in_NCList(ncid);
    if(nc == NULL) return NC_EBADID;
    if(ncpp) *ncpp = nc;
    return NC_NOERR;
}

void
free_NC(NC *ncp)
{
    if(ncp == NULL)
	return;
    if(ncp->path)
	free(ncp->path);
    /* We assume caller has already cleaned up ncp->dispatchdata */
#if _CRAYMPP && defined(LOCKNUMREC)
    shfree(ncp);
#else
    free(ncp);
#endif /* _CRAYMPP && LOCKNUMREC */
}

int
new_NC(NC_Dispatch* dispatcher, const char* path, int mode, NC** ncpp)
{
    NC *ncp = (NC*)calloc(1,sizeof(NC));
    if(ncp == NULL) return NC_ENOMEM;
    ncp->dispatch = dispatcher;
    ncp->path = nulldup(path);
    ncp->mode = mode;
    if(ncp->path == NULL) { /* fail */
        free_NC(ncp);
	return NC_ENOMEM;
    }
    if(ncpp) {
      *ncpp = ncp;
    } else {
      free_NC(ncp);
    }
    return NC_NOERR;
}

/* This function sets a default create flag that will be logically
   or'd to whatever flags are passed into nc_create for all future
   calls to nc_create.
   Valid default create flags are NC_64BIT_OFFSET, NC_CLOBBER,
   NC_LOCK, NC_SHARE. */
int
nc_set_default_format(int format, int *old_formatp)
{
    /* Return existing format if desired. */
    if (old_formatp)
      *old_formatp = default_create_format;

    /* Make sure only valid format is set. */
#ifdef USE_NETCDF4
    if (format != NC_FORMAT_CLASSIC && format != NC_FORMAT_64BIT_OFFSET &&
        format != NC_FORMAT_NETCDF4 && format != NC_FORMAT_NETCDF4_CLASSIC &&
	format != NC_FORMAT_CDF5)
      return NC_EINVAL;
 #else
    if (format != NC_FORMAT_CLASSIC && format != NC_FORMAT_64BIT_OFFSET &&
        format != NC_FORMAT_CDF5)
       return NC_EINVAL;
 #endif
    default_create_format = format;
    return NC_NOERR;
}

int
nc_get_default_format(void)
{
    return default_create_format;
}
