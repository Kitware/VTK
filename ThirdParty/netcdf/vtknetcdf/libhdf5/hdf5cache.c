/* Copyright 2018, University Corporation for Atmospheric
 * Research. See COPYRIGHT file for copying and redistribution
 * conditions. */
/**
 * @file @internal The netCDF-4 functions which control HDF5
 * caching. These caching controls allow the user to change the cache
 * sizes of HDF5 before opening files.
 *
 * @author Ed Hartnett
 */

#include "config.h"
#include "hdf5internal.h"

/* These are the default chunk cache sizes for HDF5 files created or
 * opened with netCDF-4. */
extern size_t nc4_chunk_cache_size;
extern size_t nc4_chunk_cache_nelems;
extern float nc4_chunk_cache_preemption;

/**
 * Set chunk cache size. Only affects files opened/created *after* it
 * is called.
 *
 * @param size Size in bytes to set cache.
 * @param nelems Number of elements to hold in cache.
 * @param preemption Preemption stragety (between 0 and 1).
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Bad preemption.
 * @author Ed Hartnett
 */
int
nc_set_chunk_cache(size_t size, size_t nelems, float preemption)
{
    if (preemption < 0 || preemption > 1)
        return NC_EINVAL;
    nc4_chunk_cache_size = size;
    nc4_chunk_cache_nelems = nelems;
    nc4_chunk_cache_preemption = preemption;
    return NC_NOERR;
}

/**
 * Get chunk cache size. Only affects files opened/created *after* it
 * is called.
 *
 * @param sizep Pointer that gets size in bytes to set cache.
 * @param nelemsp Pointer that gets number of elements to hold in cache.
 * @param preemptionp Pointer that gets preemption stragety (between 0 and 1).
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc_get_chunk_cache(size_t *sizep, size_t *nelemsp, float *preemptionp)
{
    if (sizep)
        *sizep = nc4_chunk_cache_size;

    if (nelemsp)
        *nelemsp = nc4_chunk_cache_nelems;

    if (preemptionp)
        *preemptionp = nc4_chunk_cache_preemption;
    return NC_NOERR;
}

/**
 * @internal Set the chunk cache. Required for fortran to avoid size_t
 * issues.
 *
 * @param size Cache size.
 * @param nelems Number of elements.
 * @param preemption Preemption * 100.
 *
 * @return NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc_set_chunk_cache_ints(int size, int nelems, int preemption)
{
    if (size <= 0 || nelems <= 0 || preemption < 0 || preemption > 100)
        return NC_EINVAL;
    nc4_chunk_cache_size = size;
    nc4_chunk_cache_nelems = nelems;
    nc4_chunk_cache_preemption = (float)preemption / 100;
    return NC_NOERR;
}

/**
 * @internal Get the chunk cache settings. Required for fortran to
 * avoid size_t issues.
 *
 * @param sizep Pointer that gets cache size.
 * @param nelemsp Pointer that gets number of elements.
 * @param preemptionp Pointer that gets preemption * 100.
 *
 * @return NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc_get_chunk_cache_ints(int *sizep, int *nelemsp, int *preemptionp)
{
    if (sizep)
        *sizep = (int)nc4_chunk_cache_size;
    if (nelemsp)
        *nelemsp = (int)nc4_chunk_cache_nelems;
    if (preemptionp)
        *preemptionp = (int)(nc4_chunk_cache_preemption * 100);

    return NC_NOERR;
}
