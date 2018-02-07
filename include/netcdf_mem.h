/*! \file netcdf_mem.h
 *
 * Main header file for in-memory (diskless) functionality.
 *
 * Copyright 2010 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
 *
 * See \ref copyright file for more info.
 *
 */

#ifndef NETCDF_MEM_H
#define NETCDF_MEM_H 1

#include <netcdf.h>

#if defined(__cplusplus)
extern "C" {
#endif

EXTERNL int nc_open_mem(const char* path, int mode, size_t size, void* memory, int* ncidp);

#if defined(__cplusplus)
}
#endif

#endif /* NETCDF_MEM_H */
