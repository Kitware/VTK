/*

This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc4 dimension functions.

Copyright 2003-5, University Corporation for Atmospheric Research. See
the COPYRIGHT file for copying and redistribution conditions.

$Id: nc4dim.c,v 1.41 2010/05/25 17:54:23 dmh Exp $
*/

#include "nc4internal.h"

#ifdef USE_PNETCDF
#include <pnetcdf.h>
#endif

/* Netcdf-4 files might have more than one unlimited dimension, but
   return the first one anyway. */
/* Note that this code is inconsistent with nc_inq */
int
NC4_inq_unlimdim(int ncid, int *unlimdimidp)
{
   NC_FILE_INFO_T *nc;
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   int found = 0;
   int retval;

   LOG((2, "called nc_inq_unlimdim"));

   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_inq_unlimdim(nc->int_ncid, unlimdimidp);
#endif /* USE_PNETCDF */

   /* Take care of netcdf-3 files. */
   assert(h5);

   /* According to netcdf-3 manual, return -1 if there is no unlimited
      dimension. */
   *unlimdimidp = -1;
   for (g = grp; g && !found; g = g->parent)
   {
      for (dim = g->dim; dim; dim = dim->next)
      {
         if (dim->unlimited)
         {
            *unlimdimidp = dim->dimid;
            found++;
            break;
         }
      }
   }

   return NC_NOERR;
}

/* Dimensions are defined in attributes attached to the appropriate
   group in the data file. */
int
NC4_def_dim(int ncid, const char *name, size_t len, int *idp)
{
   NC_FILE_INFO_T *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   char norm_name[NC_MAX_NAME + 1];
   int retval = NC_NOERR;

   LOG((2, "nc_def_dim: ncid 0x%x name %s len %d", ncid, name,
        (int)len));

   /* Find our global metadata structure. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_def_dim(nc->int_ncid, name, len, idp);
#endif /* USE_PNETCDF */

   /* Take care of netcdf-3 files. */
   assert(h5);

   assert(h5 && nc && grp);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
     return NC_EPERM;

   /* Check some stuff if strict nc3 rules are in effect. */
   if (h5->cmode & NC_CLASSIC_MODEL)
   {
      /* Only one limited dimenson for strict nc3. */
      if (len == NC_UNLIMITED)
         for (dim = grp->dim; dim; dim = dim->next)
            if (dim->unlimited)
               return NC_EUNLIMIT;

      /* Must be in define mode for stict nc3. */
      if (!(h5->flags & NC_INDEF))
         return NC_ENOTINDEFINE;
   }

   /* If it's not in define mode, enter define mode. */
   if (!(h5->flags & NC_INDEF))
      if ((retval = nc_redef(ncid)))
         return retval;

   /* Make sure this is a valid netcdf name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* For classic model, stick with the classic format restriction:
    * dim length has to fit in a 32-bit signed int. For 64-bit offset,
    * it has to fit in a 32-bit unsigned int. */
   if (h5->cmode & NC_CLASSIC_MODEL)
      if((unsigned long) len > X_INT_MAX) /* Backward compat */
         return NC_EDIMSIZE;

   /* Make sure the name is not already in use. */
   for (dim = grp->dim; dim; dim = dim->next)
      if (!strncmp(dim->name, norm_name, NC_MAX_NAME))
         return NC_ENAMEINUSE;

   /* Add a dimension to the list. The ID must come from the file
    * information, since dimids are visible in more than one group. */
   nc4_dim_list_add(&grp->dim);
   grp->dim->dimid = grp->file->nc4_info->next_dimid++;

   /* Initialize the metadata for this dimension. */
   if (!(grp->dim->name = malloc((strlen(norm_name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(grp->dim->name, norm_name);
   grp->dim->len = len;
   grp->dim->dirty++;
   if (len == NC_UNLIMITED)
      grp->dim->unlimited++;

   /* Pass back the dimid. */
   if (idp)
      *idp = grp->dim->dimid;

   return retval;
}

/* Given dim name, find its id. */
int
NC4_inq_dimid(int ncid, const char *name, int *idp)
{
   NC_FILE_INFO_T *nc;
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   char norm_name[NC_MAX_NAME + 1];
   int finished = 0;
   int retval;

   LOG((2, "nc_inq_dimid: ncid 0x%x name %s", ncid, name));

   /* Find metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_inq_dimid(nc->int_ncid, name, idp);
#endif /* USE_PNETCDF */

   /* Handle netcdf-3 files. */
   assert(h5);

   assert(nc && grp);

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   /* Go through each dim and check for a name match. */
   for (g = grp; g && !finished; g = g->parent)
      for (dim = g->dim; dim; dim = dim->next)
         if (!strncmp(dim->name, norm_name, NC_MAX_NAME))
         {
            if (idp)
               *idp = dim->dimid;
            finished++;
            return NC_NOERR;
         }

   return NC_EBADDIM;
}

/* Find out name and len of a dim. For an unlimited dimension, the
   length is the largest lenght so far written. If the name of lenp
   pointers are NULL, they will be ignored. */
int
NC4_inq_dim(int ncid, int dimid, char *name, size_t *lenp)
{
   NC_FILE_INFO_T *nc;
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp, *dim_grp;
   NC_DIM_INFO_T *dim;
   int ret = NC_NOERR;

   LOG((2, "nc_inq_dim: ncid 0x%x dimid %d", ncid, dimid));

   /* Find our global metadata structure. */
   if ((ret = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return ret;

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
   {
      MPI_Offset mpi_len;
      if ((ret = ncmpi_inq_dim(nc->int_ncid, dimid, name, &mpi_len)))
         return ret;
      if (lenp)
         *lenp = mpi_len;
   }
#endif /* USE_PNETCDF */

   /* Take care of netcdf-3 files. */
   assert(h5);

   assert(nc && grp);

   /* Find the dimension and its home group. */
   if ((ret = nc4_find_dim(grp, dimid, &dim, &dim_grp)))
      return ret;
   assert(dim);

   /* Return the dimension name, if the caller wants it. */
   if (name && dim->name)
      strcpy(name, dim->name);

   /* Return the dimension length, if the caller wants it. */
   if (lenp)
   {
      if (dim->unlimited)
      {
         /* Since this is an unlimited dimension, go to the file
            and see how many records there are. Take the max number
            of records from all the vars that share this
            dimension. */
         *lenp = 0;
         if ((ret = nc4_find_dim_len(dim_grp, dimid, &lenp)))
            return ret;
      }
      else
      {
         if (dim->too_long)
         {
            ret = NC_EDIMSIZE;
            *lenp = NC_MAX_UINT;
         }
         else
            *lenp = dim->len;
      }
   }

   return ret;
}

/* Rename a dimension, for those who like to prevaricate. */
int
NC4_rename_dim(int ncid, int dimid, const char *name)
{
   NC_FILE_INFO_T *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   if (!name)
      return NC_EINVAL;

   LOG((2, "nc_rename_dim: ncid 0x%x dimid %d name %s", ncid,
        dimid, name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc);

#ifdef USE_PNETCDF
   /* Take care of files created/opened with parallel-netcdf library. */
   if (nc->pnetcdf_file)
      return ncmpi_rename_dim(nc->int_ncid, dimid, name);
#endif /* USE_PNETCDF */

   /* Handle netcdf-3 cases. */
   assert(h5);
   assert(h5 && grp);

   /* Trying to write to a read-only file? No way, Jose! */
   if (h5->no_write)
      return NC_EPERM;

   /* Make sure this is a valid netcdf name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* Make sure the new name is not already in use in this group. */
   for (dim = grp->dim; dim; dim = dim->next)
      if (!strncmp(dim->name, norm_name, NC_MAX_NAME))
         return NC_ENAMEINUSE;

   /* Find the dim. */
   for (dim = grp->dim; dim; dim = dim->next)
      if (dim->dimid == dimid)
         break;
   if (!dim)
      return NC_EBADDIM;

   /* If not in define mode, switch to it, unless the new name is
    * shorter. (This is in accordance with the v3 interface.) */
/*    if (!(h5->flags & NC_INDEF) && strlen(name) > strlen(dim->name)) */
/*    { */
/*       if (h5->cmode & NC_CLASSIC_MODEL) */
/*       return NC_ENOTINDEFINE; */
/*       if ((retval = nc_redef(ncid))) */
/*       return retval; */
/*    } */

   /* Save the old name, we'll need it to rename this object when we
    * sync to HDF5 file. But if there already is an old_name saved,
    * just stick with what we've got, since the user might be renaming
    * the crap out of this thing, without ever syncing with the
    * file. When the sync does take place, we only need the original
    * name of the dim, not any of the intermediate ones. If the user
    * could just make up his mind, we could all get on to writing some
    * data... */
   if (!dim->old_name)
   {
      if (!(dim->old_name = malloc((strlen(dim->name) + 1) * sizeof(char))))
         return NC_ENOMEM;
      strcpy(dim->old_name, dim->name);
   }

   /* Give the dimension its new name in metadata. UTF8 normalization
    * has been done. */
   free(dim->name);
   if (!(dim->name = malloc((strlen(norm_name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(dim->name, norm_name);

   return NC_NOERR;
}

/* Returns an array of unlimited dimension ids.The user can get the
   number of unlimited dimensions by first calling this with NULL for
   the second pointer.
*/
int
NC4_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp)
{
  NC_DIM_INFO_T *dim;
  NC_GRP_INFO_T *grp;
  NC_FILE_INFO_T *nc;
  NC_HDF5_FILE_INFO_T *h5;
  int num_unlim = 0;
  int retval;

  LOG((2, "nc_inq_unlimdims: ncid 0x%x", ncid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   /* Get our dim info. */
   assert(h5);
   {
      for (dim=grp->dim; dim; dim=dim->next)
      {
         if (dim->unlimited)
         {
            if (unlimdimidsp)
               unlimdimidsp[num_unlim] = dim->dimid;
            num_unlim++;
         }
      }
   }

   /* Give the number if the user wants it. */
   if (nunlimdimsp)
      *nunlimdimsp = num_unlim;

   return NC_NOERR;
}
