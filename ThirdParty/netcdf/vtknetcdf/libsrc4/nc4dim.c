/* Copyright 2003-2018, University Corporation for Atmospheric
 * Research. See the COPYRIGHT file for copying and redistribution
 * conditions. */
/**
 * @file
 * @internal This file is part of netcdf-4, a netCDF-like interface
 * for HDF5, or a HDF5 backend for netCDF, depending on your point of
 * view.
 *
 * This file handles the nc4 dimension functions.
 *
 * @author Ed Hartnett
 */

#include "nc4internal.h"
#include "nc4dispatch.h"

/**
 * @internal Netcdf-4 files might have more than one unlimited
 * dimension, but return the first one anyway.
 *
 * @note that this code is inconsistent with nc_inq
 *
 * @param ncid File and group ID.
 * @param unlimdimidp Pointer that gets ID of first unlimited
 * dimension, or -1.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
NC4_inq_unlimdim(int ncid, int *unlimdimidp)
{
   NC *nc;
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   int found = 0;
   int retval;

   LOG((2, "%s: called", __func__));

   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(h5 && nc && grp);

   if (unlimdimidp)
   {
      /* According to netcdf-3 manual, return -1 if there is no unlimited
         dimension. */
      *unlimdimidp = -1;
      for (g = grp; g && !found; g = g->parent)
      {
         for (dim = g->dim; dim; dim = dim->l.next)
         {
            if (dim->unlimited)
            {
               *unlimdimidp = dim->dimid;
               found++;
               break;
            }
         }
      }
   }

   return NC_NOERR;
}

/**
 * @internal Dimensions are defined in attributes attached to the
 * appropriate group in the data file.
 *
 * @param ncid File and group ID.
 * @param name Name of the new dimension.
 * @param len Length of the new dimension.
 * @param idp Pointer that gets the ID of the new dimension. Ignored
 * if NULL.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EMAXNAME Name is too long.
 * @return ::NC_EBADNAME Name breaks netCDF name rules.
 * @return ::NC_EINVAL Invalid input.
 * @return ::NC_EPERM Read-only file.
 * @return ::NC_EUNLIMIT Only one unlimited dim for classic model.
 * @return ::NC_ENOTINDEFINE Not in define mode.
 * @return ::NC_EDIMSIZE Dim length too large.
 * @return ::NC_ENAMEINUSE Name already in use in group.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
NC4_def_dim(int ncid, const char *name, size_t len, int *idp)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   char norm_name[NC_MAX_NAME + 1];
   int retval = NC_NOERR;
   uint32_t nn_hash;

   LOG((2, "%s: ncid 0x%x name %s len %d", __func__, ncid, name,
        (int)len));

   /* Find our global metadata structure. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(h5 && nc && grp);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
      return NC_EPERM;

   /* Check some stuff if strict nc3 rules are in effect. */
   if (h5->cmode & NC_CLASSIC_MODEL)
   {
      /* Only one limited dimenson for strict nc3. */
      if (len == NC_UNLIMITED)
         for (dim = grp->dim; dim; dim = dim->l.next)
            if (dim->unlimited)
               return NC_EUNLIMIT;

      /* Must be in define mode for stict nc3. */
      if (!(h5->flags & NC_INDEF))
         return NC_ENOTINDEFINE;
   }

   /* Make sure this is a valid netcdf name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* For classic model: dim length has to fit in a 32-bit unsigned
    * int, as permitted for 64-bit offset format. */
   if (h5->cmode & NC_CLASSIC_MODEL)
      if(len > X_UINT_MAX) /* Backward compat */
         return NC_EDIMSIZE;

   /* Create a hash of the name. */
   nn_hash = hash_fast(norm_name, strlen(norm_name));

   /* Make sure the name is not already in use. */
   for (dim = grp->dim; dim; dim = dim->l.next)
      if (nn_hash == dim->hash && !strncmp(dim->name, norm_name, NC_MAX_NAME))
         return NC_ENAMEINUSE;

   /* If it's not in define mode, enter define mode. Do this only
    * after checking all input data, so we only enter define mode if
    * input is good. */
   if (!(h5->flags & NC_INDEF))
      if ((retval = NC4_redef(ncid)))
         return retval;

   /* Add a dimension to the list. The ID must come from the file
    * information, since dimids are visible in more than one group. */
   if ((retval = nc4_dim_list_add(&grp->dim, &dim)))
      return retval;
   dim->dimid = grp->nc4_info->next_dimid++;

   /* Initialize the metadata for this dimension. */
   if (!(dim->name = strdup(norm_name)))
      return NC_ENOMEM;
   dim->len = len;
   if (len == NC_UNLIMITED)
      dim->unlimited = NC_TRUE;

   dim->hash = nn_hash;

   /* Pass back the dimid. */
   if (idp)
      *idp = dim->dimid;

   return retval;
}

/**
 * @internal Given dim name, find its id.
 *
 * @param ncid File and group ID.
 * @param name Name of the dimension to find.
 * @param idp Pointer that gets dimension ID.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADDIM Dimension not found.
 * @return ::NC_EINVAL Invalid input. Name must be provided.
 * @author Ed Hartnett
 */
int
NC4_inq_dimid(int ncid, const char *name, int *idp)
{
   NC *nc;
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   char norm_name[NC_MAX_NAME + 1];
   int finished = 0;
   int retval;
   uint32_t shash;

   LOG((2, "%s: ncid 0x%x name %s", __func__, ncid, name));

   /* Check input. */
   if (!name)
      return NC_EINVAL;

   /* Find metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(h5 && nc && grp);

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   shash = hash_fast(norm_name, strlen(norm_name));

   /* Go through each dim and check for a name match. */
   for (g = grp; g && !finished; g = g->parent)
      for (dim = g->dim; dim; dim = dim->l.next)
         if (dim->hash == shash && !strncmp(dim->name, norm_name, NC_MAX_NAME))
         {
            if (idp)
               *idp = dim->dimid;
            return NC_NOERR;
         }

   return NC_EBADDIM;
}

/**
 * @internal Find out name and len of a dim. For an unlimited
 * dimension, the length is the largest length so far written. If the
 * name of lenp pointers are NULL, they will be ignored.
 *
 * @param ncid File and group ID.
 * @param dimid Dimension ID.
 * @param name Pointer that gets name of the dimension.
 * @param lenp Pointer that gets length of dimension.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EDIMSIZE Dimension length too large.
 * @return ::NC_EBADDIM Dimension not found.
 * @author Ed Hartnett
 */
int
NC4_inq_dim(int ncid, int dimid, char *name, size_t *lenp)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp, *dim_grp;
   NC_DIM_INFO_T *dim;
   int ret = NC_NOERR;

   LOG((2, "%s: ncid 0x%x dimid %d", __func__, ncid, dimid));

   /* Find our global metadata structure. */
   if ((ret = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return ret;
   assert(h5 && nc && grp);

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

/**
 * @internal Rename a dimension, for those who like to prevaricate.
 *
 * @param ncid File and group ID.
 * @param dimid Dimension ID.
 * @param name New dimension name.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EINVAL Name must be provided.
 * @return ::NC_ENAMEINUSE Name is already in use in group.
 * @return ::NC_EMAXNAME Name is too long.
 * @return ::NC_EBADDIM Dimension not found.
 * @return ::NC_EBADNAME Name breaks netCDF name rules.
 * @return ::NC_EDIMMETA Unable to delete HDF5 dataset.
 * @author Ed Hartnett
 */
int
NC4_rename_dim(int ncid, int dimid, const char *name)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim, *tmp_dim;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   if (!name)
      return NC_EINVAL;

   LOG((2, "%s: ncid 0x%x dimid %d name %s", __func__, ncid,
        dimid, name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc && h5 && grp);

   /* Trying to write to a read-only file? No way, Jose! */
   if (h5->no_write)
      return NC_EPERM;

   /* Make sure this is a valid netcdf name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* Check if name is in use, and retain a pointer to the correct dim */
   tmp_dim = NULL;
   for (dim = grp->dim; dim; dim = dim->l.next)
   {
      if (!strncmp(dim->name, norm_name, NC_MAX_NAME))
         return NC_ENAMEINUSE;
      if (dim->dimid == dimid)
         tmp_dim = dim;
   }
   if (!tmp_dim)
      return NC_EBADDIM;
   dim = tmp_dim;

   /* Check for renaming dimension w/o variable */
   if (dim->hdf_dimscaleid)
   {
      /* Sanity check */
      assert(!dim->coord_var);
      LOG((3, "dim %s is a dim without variable", dim->name));

      /* Delete the dimscale-only dataset. */
      if ((retval = delete_existing_dimscale_dataset(grp, dimid, dim)))
         return retval;
   }

   /* Give the dimension its new name in metadata. UTF8 normalization
    * has been done. */
   assert(dim->name);
   free(dim->name);
   if (!(dim->name = malloc((strlen(norm_name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(dim->name, norm_name);
   dim->hash = hash_fast(norm_name, strlen(norm_name));
   LOG((3, "dim is now named %s", dim->name));

   /* Check if dimension was a coordinate variable, but names are
    * different now */
   if (dim->coord_var && strcmp(dim->name, dim->coord_var->name))
   {
      /* Break up the coordinate variable */
      if ((retval = nc4_break_coord_var(grp, dim->coord_var, dim)))
         return retval;
   }

   /* Check if dimension should become a coordinate variable. */
   if (!dim->coord_var)
   {
      NC_VAR_INFO_T *var;

      /* Attempt to find a variable with the same name as the
       * dimension in the current group. */
      if ((retval = nc4_find_var(grp, dim->name, &var)))
         return retval;

      /* Check if we found a variable and the variable has the
       * dimension in index 0. */
      if (var && var->dim[0] == dim)
      {
         /* Sanity check */
         assert(var->dimids[0] == dim->dimid);

         /* Reform the coordinate variable. */
         if ((retval = nc4_reform_coord_var(grp, var, dim)))
            return retval;
      }
   }

   return NC_NOERR;
}

/**
 * @internal Returns an array of unlimited dimension ids.The user can
 * get the number of unlimited dimensions by first calling this with
 * NULL for the second pointer.
 *
 * @param ncid File and group ID.
 * @param nunlimdimsp Pointer that gets the number of unlimited
 * dimensions. Ignored if NULL.
 * @param unlimdimidsp Pointer that gets arrray of unlimited dimension
 * ID. Ignored if NULL.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp)
{
   NC_DIM_INFO_T *dim;
   NC_GRP_INFO_T *grp;
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;
   int num_unlim = 0;
   int retval;

   LOG((2, "%s: ncid 0x%x", __func__, ncid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(h5 && nc && grp);

   /* Get our dim info. */
   assert(h5);
   {
      for (dim=grp->dim; dim; dim=dim->l.next)
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
