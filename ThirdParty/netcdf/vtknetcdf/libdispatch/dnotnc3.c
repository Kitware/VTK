/* Copyright 2018, UCAR/Unidata See netcdf/COPYRIGHT file for copying
 * and redistribution conditions.*/
/**
 * @file @internal This file handles the  *_base_pe()
 * functions for dispatch layers that need to return ::NC_ENOTNC3.
 *
 * @author Ed Hartnett
*/

#include "nc4dispatch.h"

/**
 * @internal This function only does anything for netcdf-3 files.
 *
 * @param ncid Ignored.
 * @param pe Ignored.
 *
 * @return ::NC_ENOTNC3 Not a netCDF classic format file.
 * @author Ed Hartnett
 */
int
NC_NOTNC3_set_base_pe(int ncid, int pe)
{
   return NC_ENOTNC3;
}

/**
 * @internal This function only does anything for netcdf-3 files.
 *
 * @param ncid Ignored.
 * @param pe Ignored.
 *
 * @return ::NC_ENOTNC3 Not a netCDF classic format file.
 * @author Ed Hartnett
 */
int
NC_NOTNC3_inq_base_pe(int ncid, int *pe)
{
   return NC_ENOTNC3;
}

