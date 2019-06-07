/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *	See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#include <stdlib.h>
#include <stdio.h>
#include "ncstdio.h"

/* Wrappers for ncstdio dispatch methods */

int
ncstdio_uid(ncstdio* iop, int* idp)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.uid(iop,idp);
}

int
ncstdio_sync(ncstdio* iop)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.sync(iop);
}

int
ncstdio_flush(ncstdio* iop)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.flush(iop);
}

int
ncstdio_free(ncstdio* iop)
{
    if(iop == NULL) return NC_NOERR;
    return iop->ops.free(iop);
}

int
ncstdio_close(ncstdio* iop)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.close(iop);
}

int
ncstdio_seek(ncstdio* iop, off_t pos)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.seek(iop,pos);
}

int
ncstdio_read(ncstdio* iop, void* memory, const size_t size, size_t* actual)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.read(iop,memory,size,actual);
}

int
ncstdio_write(ncstdio* iop, const void* memory, const size_t size, size_t* actual)
{
    if(iop == NULL) return NC_EINVAL;
    return iop->ops.write(iop,memory,size,actual);
}
