/*

This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc_ calls, calling the appropriate nc3 or nc4
function, depending on ncid.

Copyright 2003, University Corporation for Atmospheric Research. See
netcdf-4/docs/COPYRIGHT file for copying and redistribution
conditions.

$Id: ncfunc.c,v 1.51 2010/05/26 20:13:32 dmh Exp $
*/

#include "nc4internal.h"
#include "nc3dispatch.h"

#ifdef IGNORE
/* Keep a linked list of file info objects. */
extern NC_FILE_INFO_T *nc_file;
#endif

#ifdef IGNORE
/* This function deletes a member of parliment. Be careful! Last time
 * this function was used, Labor got in! This function only does
 * anything for netcdf-3 files. */

int
nc_delete(const char *path)
{
   return NC3_delete_mp(path, 0);
}

int
nc_delete_mp(const char *path, int basepe)
{
   return NC3_delete_mp(path, basepe);
}
#endif

/* This will return the length of a netcdf data type in bytes. Since
   we haven't added any new types, I just call the v3 function.
   Ed Hartnett 10/43/03
*/

/* This function only does anything for netcdf-3 files. */
int
NC4_set_base_pe(int ncid, int pe)
{
   NC_FILE_INFO_T *nc;
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;
   if (nc->nc4_info)
      return NC_ENOTNC3;
   return NC3_set_base_pe(nc->int_ncid,  pe);
}

/* This function only does anything for netcdf-3 files. */
int
NC4_inq_base_pe(int ncid, int *pe)
{
   NC_FILE_INFO_T *nc;
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;
   if (nc->nc4_info)
      return NC_ENOTNC3;
   return NC3_inq_base_pe(nc->int_ncid, pe);
}

/* Get the format (i.e. classic, 64-bit-offset, or netcdf-4) of an
 * open file. */
int
NC4_inq_format(int ncid, int *formatp)
{
   NC_FILE_INFO_T *nc;

   LOG((2, "nc_inq_format: ncid 0x%x", ncid));

   if (!formatp)
      return NC_NOERR;

   /* Find the file metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* If this isn't a netcdf-4 file, pass this call on to the netcdf-3
    * library. */
   if (!nc->nc4_info)
      return NC3_inq_format(nc->int_ncid, formatp);
   
   /* Otherwise, this is a netcdf-4 file. Check if classic NC3 rules
    * are in effect for this file. */
   if (nc->nc4_info->cmode & NC_CLASSIC_MODEL)
      *formatp = NC_FORMAT_NETCDF4_CLASSIC;
   else
      *formatp = NC_FORMAT_NETCDF4;

   return NC_NOERR;
}


