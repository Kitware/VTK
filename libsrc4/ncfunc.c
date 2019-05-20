/*

This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc_ calls, calling the appropriate nc3 or nc4
function, depending on ncid.

Copyright 2003, University Corporation for Atmospheric Research. See
netcdf-4/docs/COPYRIGHT file for copying and redistribution
conditions.
*/

#include "nc4internal.h"
#include "nc4dispatch.h"

/* This will return the length of a netcdf data type in bytes. Since
   we haven't added any new types, I just call the v3 function.
   Ed Hartnett 10/43/03
*/

/* This function only does anything for netcdf-3 files. */
int
NC4_set_base_pe(int ncid, int pe)
{
      return NC_ENOTNC3;
}

/* This function only does anything for netcdf-3 files. */
int
NC4_inq_base_pe(int ncid, int *pe)
{
   return NC_ENOTNC3;
}

/* Get the format (i.e. classic, 64-bit-offset, or netcdf-4) of an
 * open file. */
int
NC4_inq_format(int ncid, int *formatp)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T* nc4_info;

   LOG((2, "nc_inq_format: ncid 0x%x", ncid));

   if (!formatp)
      return NC_NOERR;

   /* Find the file metadata. */
   if (!(nc = nc4_find_nc_file(ncid,&nc4_info)))
      return NC_EBADID;

   /* Otherwise, this is a netcdf-4 file. Check if classic NC3 rules
    * are in effect for this file. */
   if (nc4_info->cmode & NC_CLASSIC_MODEL)
      *formatp = NC_FORMAT_NETCDF4_CLASSIC;
   else
      *formatp = NC_FORMAT_NETCDF4;

   return NC_NOERR;
}

/* Get the extended format of an open file. */
int
NC4_inq_format_extended(int ncid, int *formatp, int *modep)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T* h5;

   LOG((2, "nc_inq_format_extended: ncid 0x%x", ncid));

   /* Find the file metadata. */
   if (!(nc = nc4_find_nc_file(ncid,&h5)))
      return NC_EBADID;

   if(modep) *modep = (nc->mode|NC_NETCDF4);

   if(formatp) {
#ifdef USE_HDF4
	/* Distinguish HDF5 from HDF4 */
	*formatp = (h5->hdf4 ? NC_FORMATX_NC_HDF4 : NC_FORMATX_NC_HDF5);
#else /* USE_HDF4 */
	*formatp = NC_FORMATX_NC_HDF5;
#endif /* USE_HDF4 */
   }
   return NC_NOERR;
}
