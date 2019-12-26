/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */
/**
 * @file
 * @internal Includes for some HDF5 stuff needed by libhdf5 code and
 * also some h5_test tests.
 *
 * @author Ed Hartnett
 */

#ifndef _NCDIMSCALE_H_
#define _NCDIMSCALE_H_

#ifdef USE_HDF5
#include <vtk_hdf5.h>

/* This is used to uniquely identify datasets, so we can keep track of
 * dimscales. */
typedef struct hdf5_objid
{
    unsigned long fileno[2]; /* file number */
    haddr_t objno[2]; /* object number */
} HDF5_OBJID_T;
#endif /* USE_HDF5 */

#endif
