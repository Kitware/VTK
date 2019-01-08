/**
 * @file
 * This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
 * HDF5 backend for netCDF, depending on your point of view.

 * This file handles the NetCDF-4 variable functions.

 * Copyright 2003-2006, University Corporation for Atmospheric
 * Research. See COPYRIGHT file for copying and redistribution
 * conditions.
 */

#include <nc4internal.h>
#include "nc4dispatch.h"
#include <math.h>

/* Szip options. */
#define NC_SZIP_EC_OPTION_MASK 4  /**< @internal SZIP EC option mask. */
#define NC_SZIP_NN_OPTION_MASK 32 /**< @internal SZIP NN option mask. */
#define NC_SZIP_MAX_PIXELS_PER_BLOCK 32 /**< @internal SZIP max pixels per block. */

/** @internal Default size for unlimited dim chunksize */
#define DEFAULT_1D_UNLIM_SIZE (4096)

#define NC_ARRAY_GROWBY 4 /**< @internal Amount to grow array. */

extern int nc4_get_default_fill_value(const NC_TYPE_INFO_T *type_info,
                                      void *fill_value);

/**
 * @internal If the HDF5 dataset for this variable is open, then close
 * it and reopen it, with the perhaps new settings for chunk caching.
 *
 * @param grp Pointer to the group info.
 * @param var Pointer to the var info.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EHDFERR HDF5 error.
 * @author Ed Hartnett
 */
int
nc4_reopen_dataset(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
   hid_t access_pid;

   if (var->hdf_datasetid)
   {
      if ((access_pid = H5Pcreate(H5P_DATASET_ACCESS)) < 0)
         return NC_EHDFERR;
      if (H5Pset_chunk_cache(access_pid, var->chunk_cache_nelems,
                             var->chunk_cache_size,
                             var->chunk_cache_preemption) < 0)
         return NC_EHDFERR;
      if (H5Dclose(var->hdf_datasetid) < 0)
         return NC_EHDFERR;
      if ((var->hdf_datasetid = H5Dopen2(grp->hdf_grpid, var->name,
                                         access_pid)) < 0)
         return NC_EHDFERR;
      if (H5Pclose(access_pid) < 0)
         return NC_EHDFERR;
   }

   return NC_NOERR;
}

/**
 * @internal Set chunk cache size for a variable. This is the internal
 * function called by nc_set_var_chunk_cache().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param size Size in bytes to set cache.
 * @param nelems Number of elements in cache.
 * @param preemption Controls cache swapping.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3 netcdf-4 file.
 * @returns ::NC_EINVAL Invalid input.
 * @returns ::NC_EHDFERR HDF5 error.
 * @author Ed Hartnett
 */
int
NC4_set_var_chunk_cache(int ncid, int varid, size_t size, size_t nelems,
                        float preemption)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   int retval;

   /* Check input for validity. */
   if (preemption < 0 || preemption > 1)
      return NC_EINVAL;

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc && grp && h5);

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   assert(var && var->varid == varid);

   /* Set the values. */
   var->chunk_cache_size = size;
   var->chunk_cache_nelems = nelems;
   var->chunk_cache_preemption = preemption;

   if ((retval = nc4_reopen_dataset(grp, var)))
      return retval;

   return NC_NOERR;
}

/**
 * @internal A wrapper for NC4_set_var_chunk_cache(), we need this
 * version for fortran. Negative values leave settings as they are.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param size Size in bytes to set cache.
 * @param nelems Number of elements in cache.
 * @param preemption Controls cache swapping.
 *
 * @returns ::NC_NOERR for success
 * @author Ed Hartnett
 */
int
nc_set_var_chunk_cache_ints(int ncid, int varid, int size, int nelems,
                            int preemption)
{
   size_t real_size = H5D_CHUNK_CACHE_NBYTES_DEFAULT;
   size_t real_nelems = H5D_CHUNK_CACHE_NSLOTS_DEFAULT;
   float real_preemption = CHUNK_CACHE_PREEMPTION;

   if (size >= 0)
      real_size = ((size_t) size) * MEGABYTE;

   if (nelems >= 0)
      real_nelems = nelems;

   if (preemption >= 0)
      real_preemption = preemption / 100.;

   return NC4_set_var_chunk_cache(ncid, varid, real_size, real_nelems,
                                  real_preemption);
}

/**
 * @internal This is called by nc_get_var_chunk_cache(). Get chunk
 * cache size for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param sizep Gets size in bytes of cache.
 * @param nelemsp Gets number of element slots in cache.
 * @param preemptionp Gets cache swapping setting.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Not a netCDF-4 file.
 * @author Ed Hartnett
 */
int
NC4_get_var_chunk_cache(int ncid, int varid, size_t *sizep,
                        size_t *nelemsp, float *preemptionp)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   int retval;

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc && grp && h5);

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   assert(var && var->varid == varid);

   /* Give the user what they want. */
   if (sizep)
      *sizep = var->chunk_cache_size;
   if (nelemsp)
      *nelemsp = var->chunk_cache_nelems;
   if (preemptionp)
      *preemptionp = var->chunk_cache_preemption;

   return NC_NOERR;
}

/**
 * @internal A wrapper for NC4_get_var_chunk_cache(), we need this
 * version for fortran.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param sizep Gets size in bytes of cache.
 * @param nelemsp Gets number of element slots in cache.
 * @param preemptionp Gets cache swapping setting.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Not a netCDF-4 file.
 * @author Ed Hartnett
 */
int
nc_get_var_chunk_cache_ints(int ncid, int varid, int *sizep,
                            int *nelemsp, int *preemptionp)
{
   size_t real_size, real_nelems;
   float real_preemption;
   int ret;

   if ((ret = NC4_get_var_chunk_cache(ncid, varid, &real_size,
                                      &real_nelems, &real_preemption)))
      return ret;

   if (sizep)
      *sizep = real_size / MEGABYTE;
   if (nelemsp)
      *nelemsp = (int)real_nelems;
   if(preemptionp)
      *preemptionp = (int)(real_preemption * 100);

   return NC_NOERR;
}

/**
 * @internal Check a set of chunksizes to see if they specify a chunk
 * that is too big.
 *
 * @param grp Pointer to the group info.
 * @param var Pointer to the var info.
 * @param chunksizes Array of chunksizes to check.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_EBADCHUNK Bad chunksize.
 */
static int
check_chunksizes(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var, const size_t *chunksizes)
{
   double dprod;
   size_t type_len;
   int d;
   int retval;

   if ((retval = nc4_get_typelen_mem(grp->nc4_info, var->type_info->nc_typeid, 0, &type_len)))
      return retval;
   if (var->type_info->nc_type_class == NC_VLEN)
      dprod = (double)sizeof(hvl_t);
   else
      dprod = (double)type_len;
   for (d = 0; d < var->ndims; d++)
      dprod *= (double)chunksizes[d];

   if (dprod > (double) NC_MAX_UINT)
      return NC_EBADCHUNK;

   return NC_NOERR;
}

/**
 * @internal Determine some default chunksizes for a variable.
 *
 * @param grp Pointer to the group info.
 * @param var Pointer to the var info.
 *
 * @returns ::NC_NOERR for success
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @author Ed Hartnett
 */
static int
nc4_find_default_chunksizes2(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
   int d;
   size_t type_size;
   float num_values = 1, num_unlim = 0;
   int retval;
   size_t suggested_size;
#ifdef LOGGING
   double total_chunk_size;
#endif

   if (var->type_info->nc_type_class == NC_STRING)
      type_size = sizeof(char *);
   else
      type_size = var->type_info->size;

#ifdef LOGGING
   /* Later this will become the total number of bytes in the default
    * chunk. */
   total_chunk_size = (double) type_size;
#endif

   /* How many values in the variable (or one record, if there are
    * unlimited dimensions). */
   for (d = 0; d < var->ndims; d++)
   {
      assert(var->dim[d]);
      if (! var->dim[d]->unlimited)
         num_values *= (float)var->dim[d]->len;
      else {
         num_unlim++;
         var->chunksizes[d] = 1; /* overwritten below, if all dims are unlimited */
      }
   }
   /* Special case to avoid 1D vars with unlim dim taking huge amount
      of space (DEFAULT_CHUNK_SIZE bytes). Instead we limit to about
      4KB */
   if (var->ndims == 1 && num_unlim == 1) {
      if (DEFAULT_CHUNK_SIZE / type_size <= 0)
         suggested_size = 1;
      else if (DEFAULT_CHUNK_SIZE / type_size > DEFAULT_1D_UNLIM_SIZE)
         suggested_size = DEFAULT_1D_UNLIM_SIZE;
      else
         suggested_size = DEFAULT_CHUNK_SIZE / type_size;
      var->chunksizes[0] = suggested_size / type_size;
      LOG((4, "%s: name %s dim %d DEFAULT_CHUNK_SIZE %d num_values %f type_size %d "
           "chunksize %ld", __func__, var->name, d, DEFAULT_CHUNK_SIZE, num_values, type_size, var->chunksizes[0]));
   }
   if (var->ndims > 1 && var->ndims == num_unlim) { /* all dims unlimited */
      suggested_size = pow((double)DEFAULT_CHUNK_SIZE/type_size, 1.0/(double)(var->ndims));
      for (d = 0; d < var->ndims; d++)
      {
         var->chunksizes[d] = suggested_size ? suggested_size : 1;
         LOG((4, "%s: name %s dim %d DEFAULT_CHUNK_SIZE %d num_values %f type_size %d "
              "chunksize %ld", __func__, var->name, d, DEFAULT_CHUNK_SIZE, num_values, type_size, var->chunksizes[d]));
      }
   }

   /* Pick a chunk length for each dimension, if one has not already
    * been picked above. */
   for (d = 0; d < var->ndims; d++)
      if (!var->chunksizes[d])
      {
         suggested_size = (pow((double)DEFAULT_CHUNK_SIZE/(num_values * type_size),
                               1.0/(double)(var->ndims - num_unlim)) * var->dim[d]->len - .5);
         if (suggested_size > var->dim[d]->len)
            suggested_size = var->dim[d]->len;
         var->chunksizes[d] = suggested_size ? suggested_size : 1;
         LOG((4, "%s: name %s dim %d DEFAULT_CHUNK_SIZE %d num_values %f type_size %d "
              "chunksize %ld", __func__, var->name, d, DEFAULT_CHUNK_SIZE, num_values, type_size, var->chunksizes[d]));
      }

#ifdef LOGGING
   /* Find total chunk size. */
   for (d = 0; d < var->ndims; d++)
      total_chunk_size *= (double) var->chunksizes[d];
   LOG((4, "total_chunk_size %f", total_chunk_size));
#endif

   /* But did this result in a chunk that is too big? */
   retval = check_chunksizes(grp, var, var->chunksizes);
   if (retval)
   {
      /* Other error? */
      if (retval != NC_EBADCHUNK)
         return retval;

      /* Chunk is too big! Reduce each dimension by half and try again. */
      for ( ; retval == NC_EBADCHUNK; retval = check_chunksizes(grp, var, var->chunksizes))
         for (d = 0; d < var->ndims; d++)
            var->chunksizes[d] = var->chunksizes[d]/2 ? var->chunksizes[d]/2 : 1;
   }

   /* Do we have any big data overhangs? They can be dangerous to
    * babies, the elderly, or confused campers who have had too much
    * beer. */
   for (d = 0; d < var->ndims; d++)
   {
      size_t num_chunks;
      size_t overhang;
      assert(var->chunksizes[d] > 0);
      num_chunks = (var->dim[d]->len + var->chunksizes[d] - 1) / var->chunksizes[d];
      if(num_chunks > 0) {
         overhang = (num_chunks * var->chunksizes[d]) - var->dim[d]->len;
         var->chunksizes[d] -= overhang / num_chunks;
      }
   }

   return NC_NOERR;
}

/**
 * @internal Grow the variable array.
 *
 * @param grp Pointer to the group info.
 * @param var Pointer to the var info.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_ENOMEM Out of memory.
 * @author Dennis Heimbigner
 */
int
nc4_vararray_add(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
   NC_VAR_INFO_T **vp = NULL;

   if (grp->vars.nalloc == 0) {
      assert(grp->vars.nelems == 0);
      vp = (NC_VAR_INFO_T **) malloc(NC_ARRAY_GROWBY * sizeof(NC_VAR_INFO_T *));
      if(vp == NULL)
         return NC_ENOMEM;
      grp->vars.value = vp;
      grp->vars.nalloc = NC_ARRAY_GROWBY;
   }
   else if(grp->vars.nelems +1 > grp->vars.nalloc) {
      vp = (NC_VAR_INFO_T **) realloc(grp->vars.value,
                                      (grp->vars.nalloc + NC_ARRAY_GROWBY) * sizeof(NC_VAR_INFO_T *));
      if(vp == NULL)
         return NC_ENOMEM;
      grp->vars.value = vp;
      grp->vars.nalloc += NC_ARRAY_GROWBY;
   }

   assert(var->varid == grp->vars.nelems);
   grp->vars.value[grp->vars.nelems] = var;
   grp->vars.nelems++;

   return NC_NOERR;
}

/**
 * @internal This is called when a new netCDF-4 variable is defined
 * with nc_def_var().
 *
 * @param ncid File ID.
 * @param name Name.
 * @param xtype Type.
 * @param ndims Number of dims.
 * @param dimidsp Array of dim IDs.
 * @param varidp Gets the var ID.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
 * netcdf-4 file.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EPERM File is read only.
 * @returns ::NC_EMAXDIMS Classic model file exceeds ::NC_MAX_VAR_DIMS.
 * @returns ::NC_ESTRICTNC3 Attempting to create netCDF-4 type var in
 * classic model file
 * @returns ::NC_EBADNAME Bad name.
 * @returns ::NC_EBADTYPE Bad type.
 * @returns ::NC_ENOMEM Out of memory.
 * @returns ::NC_EHDFERR Error returned by HDF5 layer.
 * @returns ::NC_EINVAL Invalid input
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_def_var(int ncid, const char *name, nc_type xtype,
            int ndims, const int *dimidsp, int *varidp)
{
   NC_GRP_INFO_T *grp;
   NC_VAR_INFO_T *var;
   NC_DIM_INFO_T *dim;
   NC_HDF5_FILE_INFO_T *h5;
   NC_TYPE_INFO_T *type_info = NULL;
   char norm_name[NC_MAX_NAME + 1];
   int d;
   int retval;

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      BAIL(retval);
   assert(grp && h5);

   /* If it's not in define mode, strict nc3 files error out,
    * otherwise switch to define mode. This will also check that the
    * file is writable. */
   if (!(h5->flags & NC_INDEF))
   {
      if (h5->cmode & NC_CLASSIC_MODEL)
         BAIL(NC_ENOTINDEFINE);
      if ((retval = NC4_redef(ncid)))
         BAIL(retval);
   }
   assert(!h5->no_write);

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      BAIL(retval);

   /* Not a Type is, well, not a type.*/
   if (xtype == NC_NAT)
      BAIL(NC_EBADTYPE);

   /* For classic files, only classic types are allowed. */
   if (h5->cmode & NC_CLASSIC_MODEL && xtype > NC_DOUBLE)
      BAIL(NC_ESTRICTNC3);

   /* For classic files */
   if (h5->cmode & NC_CLASSIC_MODEL && ndims > NC_MAX_VAR_DIMS)
      BAIL(NC_EMAXDIMS);

   /* cast needed for braindead systems with signed size_t */
   if((unsigned long) ndims > X_INT_MAX) /* Backward compat */
      BAIL(NC_EINVAL);

   /* Check that this name is not in use as a var, grp, or type. */
   if ((retval = nc4_check_dup_name(grp, norm_name)))
      BAIL(retval);

   /* For non-scalar vars, dim IDs must be provided. */
   if (ndims && !dimidsp)
      BAIL(NC_EINVAL);      

   /* Check all the dimids to make sure they exist. */
   for (d = 0; d < ndims; d++)
      if ((retval = nc4_find_dim(grp, dimidsp[d], &dim, NULL)))
         BAIL(retval);

   /* These degrubbing messages sure are handy! */
   LOG((2, "%s: name %s type %d ndims %d", __func__, norm_name, xtype, ndims));
#ifdef LOGGING
   {
      int dd;
      for (dd = 0; dd < ndims; dd++)
         LOG((4, "dimid[%d] %d", dd, dimidsp[dd]));
   }
#endif

   /* If this is a user-defined type, there is a type_info struct with
    * all the type information. For atomic types, fake up a type_info
    * struct. */
   if (xtype <= NC_STRING)
   {
      if (!(type_info = calloc(1, sizeof(NC_TYPE_INFO_T))))
         BAIL(NC_ENOMEM);
      type_info->nc_typeid = xtype;
      type_info->endianness = NC_ENDIAN_NATIVE;
      if ((retval = nc4_get_hdf_typeid(h5, xtype, &type_info->hdf_typeid,
                                       type_info->endianness)))
         BAIL(retval);
      if ((type_info->native_hdf_typeid = H5Tget_native_type(type_info->hdf_typeid,
                                                             H5T_DIR_DEFAULT)) < 0)
         BAIL(NC_EHDFERR);
      if ((retval = nc4_get_typelen_mem(h5, type_info->nc_typeid, 0,
                                        &type_info->size)))
         BAIL(retval);

      /* Set the "class" of the type */
      if (xtype == NC_CHAR)
         type_info->nc_type_class = NC_CHAR;
      else
      {
         H5T_class_t class;

         if ((class = H5Tget_class(type_info->hdf_typeid)) < 0)
            BAIL(NC_EHDFERR);
         switch(class)
         {
         case H5T_STRING:
            type_info->nc_type_class = NC_STRING;
            break;

         case H5T_INTEGER:
            type_info->nc_type_class = NC_INT;
            break;

         case H5T_FLOAT:
            type_info->nc_type_class = NC_FLOAT;
            break;

         default:
            BAIL(NC_EBADTYPID);
         }
      }
   }
   /* If this is a user defined type, find it. */
   else
   {
      if (nc4_find_type(grp->nc4_info, xtype, &type_info))
         BAIL(NC_EBADTYPE);
   }

   /* Create a new var and fill in some HDF5 cache setting values. */
   if ((retval = nc4_var_add(&var)))
      BAIL(retval);

   /* Now fill in the values in the var info structure. */
   if (!(var->name = malloc((strlen(norm_name) + 1) * sizeof(char))))
      BAIL(NC_ENOMEM);
   strcpy(var->name, norm_name);
   var->hash = hash_fast(norm_name, strlen(norm_name));
   var->varid = grp->nvars++;
   var->ndims = ndims;
   var->is_new_var = NC_TRUE;

   /* Add a var to the variable array, growing it as needed. */
   if ((retval = nc4_vararray_add(grp, var)))
      BAIL(retval);

   /* Point to the type, and increment its ref. count */
   var->type_info = type_info;
   var->type_info->rc++;
   type_info = NULL;

   /* Allocate space for dimension information. */
   if (ndims)
   {
      if (!(var->dim = calloc(ndims, sizeof(NC_DIM_INFO_T *))))
         BAIL(NC_ENOMEM);
      if (!(var->dimids = calloc(ndims, sizeof(int))))
         BAIL(NC_ENOMEM);
   }

   /* Set variables no_fill to match the database default
    * unless the variable type is variable length (NC_STRING or NC_VLEN)
    * or is user-defined type.
    */
   if (var->type_info->nc_type_class < NC_STRING)
      var->no_fill = h5->fill_mode;

   /* Assign dimensions to the variable */
   /* At the same time, check to see if this is a coordinate
    * variable. If so, it will have the same name as one of its
    * dimensions. If it is a coordinate var, is it a coordinate var in
    * the same group as the dim? */
   /* Also, check whether we should use contiguous or chunked storage */
   var->contiguous = NC_TRUE;
   for (d = 0; d < ndims; d++)
   {
      NC_GRP_INFO_T *dim_grp;

      /* Look up each dimension */
      if ((retval = nc4_find_dim(grp, dimidsp[d], &dim, &dim_grp)))
         BAIL(retval);

      /* Check for dim index 0 having the same name, in the same group */
      if (d == 0 && dim_grp == grp && dim->hash == var->hash && strcmp(dim->name, norm_name) == 0)
      {
         var->dimscale = NC_TRUE;
         dim->coord_var = var;
         
         /* Use variable's dataset ID for the dimscale ID. So delete
          * the HDF5 DIM_WITHOUT_VARIABLE dataset that was created for
          * this dim. */
         if (dim->hdf_dimscaleid)
         {
            /* Detach dimscale from any variables using it */
            if ((retval = rec_detach_scales(grp, dimidsp[d], dim->hdf_dimscaleid)) < 0)
               BAIL(retval);
            
            /* Close the HDF5 DIM_WITHOUT_VARIABLE dataset. */
            if (H5Dclose(dim->hdf_dimscaleid) < 0)
               BAIL(NC_EHDFERR);
            dim->hdf_dimscaleid = 0;
            
            /* Now delete the DIM_WITHOUT_VARIABLE dataset (it will be
             * recreated later, if necessary). */
            if (H5Gunlink(grp->hdf_grpid, dim->name) < 0)
               BAIL(NC_EDIMMETA);
         }
      }

      /* Check for unlimited dimension and turn off contiguous storage. */
      if (dim->unlimited)
         var->contiguous = NC_FALSE;

      /* Track dimensions for variable */
      var->dimids[d] = dimidsp[d];
      var->dim[d] = dim;
   }

   /* Determine default chunksizes for this variable. (Even for
    * variables which may be contiguous.) */
   LOG((4, "allocating array of %d size_t to hold chunksizes for var %s",
        var->ndims, var->name));
   if (var->ndims)
      if (!(var->chunksizes = calloc(var->ndims, sizeof(size_t))))
         BAIL(NC_ENOMEM);

   if ((retval = nc4_find_default_chunksizes2(grp, var)))
      BAIL(retval);

   /* Is this a variable with a chunksize greater than the current
    * cache size? */
   if ((retval = nc4_adjust_var_cache(grp, var)))
      BAIL(retval);

   /* If the user names this variable the same as a dimension, but
    * doesn't use that dimension first in its list of dimension ids,
    * is not a coordinate variable. I need to change its HDF5 name,
    * because the dimension will cause a HDF5 dataset to be created,
    * and this var has the same name. */
   for (dim = grp->dim; dim; dim = dim->l.next)
      if (dim->hash == var->hash && !strcmp(dim->name, norm_name) &&
          (!var->ndims || dimidsp[0] != dim->dimid))
      {
         /* Set a different hdf5 name for this variable to avoid name
          * clash. */
         if (strlen(norm_name) + strlen(NON_COORD_PREPEND) > NC_MAX_NAME)
            BAIL(NC_EMAXNAME);
         if (!(var->hdf5_name = malloc((strlen(NON_COORD_PREPEND) +
                                        strlen(norm_name) + 1) * sizeof(char))))
            BAIL(NC_ENOMEM);

         sprintf(var->hdf5_name, "%s%s", NON_COORD_PREPEND, norm_name);
      }

   /* If this is a coordinate var, it is marked as a HDF5 dimension
    * scale. (We found dim above.) Otherwise, allocate space to
    * remember whether dimension scales have been attached to each
    * dimension. */
   if (!var->dimscale && ndims)
      if (!(var->dimscale_attached = calloc(ndims, sizeof(nc_bool_t))))
         BAIL(NC_ENOMEM);

   /* Return the varid. */
   if (varidp)
      *varidp = var->varid;
   LOG((4, "new varid %d", var->varid));

exit:
   if (type_info)
      if ((retval = nc4_type_free(type_info)))
         BAIL2(retval);

   return retval;
}

/**
 * @internal Get all the information about a variable. Pass NULL for
 * whatever you don't care about. This is the internal function called
 * by nc_inq_var(), nc_inq_var_deflate(), nc_inq_var_fletcher32(),
 * nc_inq_var_chunking(), nc_inq_var_chunking_ints(),
 * nc_inq_var_fill(), nc_inq_var_endian(), nc_inq_var_filter(), and
 * nc_inq_var_szip().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param name Gets name.
 * @param xtypep Gets type.
 * @param ndimsp Gets number of dims.
 * @param dimidsp Gets array of dim IDs.
 * @param nattsp Gets number of attributes.
 * @param shufflep Gets shuffle setting.
 * @param deflatep Gets deflate setting.
 * @param deflate_levelp Gets deflate level.
 * @param fletcher32p Gets fletcher32 setting.
 * @param contiguousp Gets contiguous setting.
 * @param chunksizesp Gets chunksizes.
 * @param no_fill Gets fill mode.
 * @param fill_valuep Gets fill value.
 * @param endiannessp Gets one of ::NC_ENDIAN_BIG ::NC_ENDIAN_LITTLE
 * ::NC_ENDIAN_NATIVE
 * @param idp Pointer to memory to store filter id.
 * @param nparamsp Pointer to memory to store filter parameter count.
 * @param params Pointer to vector of unsigned integers into which
 * to store filter parameters.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Bad varid.
 * @returns ::NC_ENOMEM Out of memory.
 * @returns ::NC_EINVAL Invalid input.
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
                int *ndimsp, int *dimidsp, int *nattsp,
                int *shufflep, int *deflatep, int *deflate_levelp,
                int *fletcher32p, int *contiguousp, size_t *chunksizesp,
                int *no_fill, void *fill_valuep, int *endiannessp,
                unsigned int* idp, size_t* nparamsp, unsigned int* params
   )
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T *att;
   int natts=0;
   int d;
   int retval;

   LOG((2, "%s: ncid 0x%x varid %d", __func__, ncid, varid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(nc);
   assert(grp && h5);

   /* Walk through the list of vars, and return the info about the one
      with a matching varid. If the varid is -1, find the global
      atts and call it a day. */
   if (varid == NC_GLOBAL)
   {
      if (nattsp)
      {
         for (att = grp->att; att; att = att->l.next)
            natts++;
         *nattsp = natts;
      }
      return NC_NOERR;
   }

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   assert(var && var->varid == varid);

   /* Copy the data to the user's data buffers. */
   if (name)
      strcpy(name, var->name);
   if (xtypep)
      *xtypep = var->type_info->nc_typeid;
   if (ndimsp)
      *ndimsp = var->ndims;
   if (dimidsp)
      for (d = 0; d < var->ndims; d++)
         dimidsp[d] = var->dimids[d];
   if (nattsp)
   {
      for (att = var->att; att; att = att->l.next)
         natts++;
      *nattsp = natts;
   }

   /* Chunking stuff. */
   if (!var->contiguous && chunksizesp)
      for (d = 0; d < var->ndims; d++)
      {
         chunksizesp[d] = var->chunksizes[d];
         LOG((4, "chunksizesp[%d]=%d", d, chunksizesp[d]));
      }

   if (contiguousp)
      *contiguousp = var->contiguous ? NC_CONTIGUOUS : NC_CHUNKED;

   /* Filter stuff. */
   if (deflatep)
      *deflatep = (int)var->deflate;
   if (deflate_levelp)
      *deflate_levelp = var->deflate_level;
   if (shufflep)
      *shufflep = (int)var->shuffle;
   if (fletcher32p)
      *fletcher32p = (int)var->fletcher32;

   if (idp)
      *idp = var->filterid;
   if (nparamsp)
      *nparamsp = (var->params == NULL ? 0 : var->nparams);
   if (params && var->params != NULL)
      memcpy(params,var->params,var->nparams*sizeof(unsigned int));

   /* Fill value stuff. */
   if (no_fill)
      *no_fill = (int)var->no_fill;

   /* Don't do a thing with fill_valuep if no_fill mode is set for
    * this var, or if fill_valuep is NULL. */
   if (!var->no_fill && fill_valuep)
   {
      /* Do we have a fill value for this var? */
      if (var->fill_value)
      {
         if (var->type_info->nc_type_class == NC_STRING)
         {
            assert(*(char **)var->fill_value);
            /* This will allocate memeory and copy the string. */
            if (!(*(char **)fill_valuep = strdup(*(char **)var->fill_value)))
            {
               free(*(char **)fill_valuep);
               return NC_ENOMEM;
            }
         }
         else
         {
            assert(var->type_info->size);
            memcpy(fill_valuep, var->fill_value, var->type_info->size);
         }
      }
      else
      {
         if (var->type_info->nc_type_class == NC_STRING)
         {
            if (!(*(char **)fill_valuep = calloc(1, sizeof(char *))))
               return NC_ENOMEM;

            if ((retval = nc4_get_default_fill_value(var->type_info, (char **)fill_valuep)))
            {
               free(*(char **)fill_valuep);
               return retval;
            }
         }
         else
         {
            if ((retval = nc4_get_default_fill_value(var->type_info, fill_valuep)))
               return retval;
         }
      }
   }

   /* Does the user want the endianness of this variable? */
   if (endiannessp)
      *endiannessp = var->type_info->endianness;

   return NC_NOERR;
}

/**
 * @internal This functions sets extra stuff about a netCDF-4 variable which
 * must be set before the enddef but after the def_var.
 *
 * @note All pointer parameters may be NULL, in which case they are ignored.
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param shuffle Pointer to shuffle setting. 
 * @param deflate Pointer to deflate setting.
 * @param deflate_level Pointer to deflate level.
 * @param fletcher32 Pointer to fletcher32 setting.
 * @param contiguous Pointer to contiguous setting.
 * @param chunksizes Array of chunksizes.
 * @param no_fill Pointer to no_fill setting.
 * @param fill_value Pointer to fill value.
 * @param endianness Pointer to endianness setting.
 *
 * @returns ::NC_NOERR for success
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
 * netcdf-4 file.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EPERM File is read only.
 * @returns ::NC_EINVAL Invalid input
 * @returns ::NC_EBADCHUNK Bad chunksize.
 * @author Ed Hartnett
 */
static int
nc_def_var_extra(int ncid, int varid, int *shuffle, int *deflate,
                 int *deflate_level, int *fletcher32, int *contiguous,
                 const size_t *chunksizes, int *no_fill,
                 const void *fill_value, int *endianness)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   int d;
   int retval;

   /* All or none of these will be provided. */
   assert((deflate && deflate_level && shuffle) ||
          (!deflate && !deflate_level && !shuffle));

   LOG((2, "%s: ncid 0x%x varid %d", __func__, ncid, varid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc && grp && h5);

   /* Trying to write to a read-only file? No way, Jose! */
   if (h5->no_write)
      return NC_EPERM;

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   assert(var && var->varid == varid);

   /* Can't turn on parallel and deflate/fletcher32/szip/shuffle. */
   if (nc->mode & (NC_MPIIO | NC_MPIPOSIX)) {
      if (deflate || fletcher32 || shuffle)
         return NC_EINVAL;
   }

   /* If the HDF5 dataset has already been created, then it is too
    * late to set all the extra stuff. */
   if (var->created)
      return NC_ELATEDEF;

   /* Check compression options. */
   if (deflate && !deflate_level)
      return NC_EINVAL;

   /* Valid deflate level? */
   if (deflate)
   {
      if (*deflate)
         if (*deflate_level < NC_MIN_DEFLATE_LEVEL ||
             *deflate_level > NC_MAX_DEFLATE_LEVEL)
            return NC_EINVAL;

      /* For scalars, just ignore attempt to deflate. */
      if (!var->ndims)
         return NC_NOERR;

      /* Well, if we couldn't find any errors, I guess we have to take
       * the users settings. Darn! */
      var->contiguous = NC_FALSE;
      var->deflate = *deflate;
      if (*deflate)
         var->deflate_level = *deflate_level;
      LOG((3, "%s: *deflate_level %d", __func__, *deflate_level));
   }

   /* Shuffle filter? */
   if (shuffle)
   {
      var->shuffle = *shuffle;
      var->contiguous = NC_FALSE;
   }

   /* Fletcher32 checksum error protection? */
   if (fletcher32)
   {
      var->fletcher32 = *fletcher32;
      var->contiguous = NC_FALSE;
   }

   /* Does the user want a contiguous dataset? Not so fast! Make sure
    * that there are no unlimited dimensions, and no filters in use
    * for this data. */
   if (contiguous && *contiguous)
   {
      if (var->deflate || var->fletcher32 || var->shuffle)
         return NC_EINVAL;

      for (d = 0; d < var->ndims; d++)
         if (var->dim[d]->unlimited)
            return NC_EINVAL;
      var->contiguous = NC_TRUE;
   }

   /* Chunksizes anyone? */
   if (contiguous && *contiguous == NC_CHUNKED)
   {
      var->contiguous = NC_FALSE;

      /* If the user provided chunksizes, check that they are not too
       * big, and that their total size of chunk is less than 4 GB. */
      if (chunksizes)
      {

         if ((retval = check_chunksizes(grp, var, chunksizes)))
            return retval;

         /* Ensure chunksize is smaller than dimension size */
         for (d = 0; d < var->ndims; d++)
            if(!var->dim[d]->unlimited && var->dim[d]->len > 0 && chunksizes[d] > var->dim[d]->len)
               return NC_EBADCHUNK;

         /* Set the chunksizes for this variable. */
         for (d = 0; d < var->ndims; d++)
            var->chunksizes[d] = chunksizes[d];
      }
   }

   /* Is this a variable with a chunksize greater than the current
    * cache size? */
   if (!var->contiguous && (deflate || contiguous))
   {
      /* Determine default chunksizes for this variable (do nothing
       * for scalar vars). */
      if (var->chunksizes && !var->chunksizes[0])
         if ((retval = nc4_find_default_chunksizes2(grp, var)))
            return retval;

      /* Adjust the cache. */
      if ((retval = nc4_adjust_var_cache(grp, var)))
         return retval;
   }

   /* Are we setting a fill modes? */
   if (no_fill)
   {
      if (*no_fill)
      {
         /* NC_STRING types may not turn off fill mode. It's disallowed
          * by HDF5 and will cause a HDF5 error later. */
         if (*no_fill)
            if (var->type_info->nc_typeid == NC_STRING)
               return NC_EINVAL;

         /* Set the no-fill mode. */
         var->no_fill = NC_TRUE;
      }
      else
         var->no_fill = NC_FALSE;
   }

   /* Are we setting a fill value? */
   if (fill_value && !var->no_fill)
   {
      /* Copy the fill_value. */
      LOG((4, "Copying fill value into metadata for variable %s",
           var->name));

      /* If there's a _FillValue attribute, delete it. */
      retval = NC4_del_att(ncid, varid, _FillValue);
      if (retval && retval != NC_ENOTATT)
         return retval;

      /* Create a _FillValue attribute. */
      if ((retval = nc_put_att(ncid, varid, _FillValue, var->type_info->nc_typeid, 1, fill_value)))
         return retval;
   }

   /* Is the user setting the endianness? */
   if (endianness)
      var->type_info->endianness = *endianness;

   return NC_NOERR;
}

/**
 * @internal Set compression settings on a variable. This is called by
 * nc_def_var_deflate().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param shuffle True to turn on the shuffle filter.
 * @param deflate True to turn on deflation.
 * @param deflate_level A number between 0 (no compression) and 9
 * (maximum compression).
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EINVAL Invalid input
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_def_var_deflate(int ncid, int varid, int shuffle, int deflate,
                    int deflate_level)
{
   return nc_def_var_extra(ncid, varid, &shuffle, &deflate,
                           &deflate_level, NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
 * @internal Set checksum on a variable. This is called by
 * nc_def_var_fletcher32().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param fletcher32 Pointer to fletcher32 setting.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EINVAL Invalid input
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_def_var_fletcher32(int ncid, int varid, int fletcher32)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, &fletcher32,
                           NULL, NULL, NULL, NULL, NULL);
}

/**
 * @internal Define chunking stuff for a var. This is called by
 * nc_def_var_chunking(). Chunking is required in any dataset with one
 * or more unlimited dimensions in HDF5, or any dataset using a
 * filter.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param contiguous Pointer to contiguous setting.
 * @param chunksizesp Array of chunksizes.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EINVAL Invalid input
 * @returns ::NC_EBADCHUNK Bad chunksize.
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_def_var_chunking(int ncid, int varid, int contiguous, const size_t *chunksizesp)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL,
                           &contiguous, chunksizesp, NULL, NULL, NULL);
}

/**
 * @internal Inquire about chunking settings for a var. This is used
 * by the fortran API.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param contiguousp Gets contiguous setting.
 * @param chunksizesp Gets chunksizes.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_EINVAL Invalid input
 * @returns ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
nc_inq_var_chunking_ints(int ncid, int varid, int *contiguousp, int *chunksizesp)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_VAR_INFO_T *var;
   NC_HDF5_FILE_INFO_T *h5;

   size_t *cs = NULL;
   int i, retval;

   /* Find this ncid's file info. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc);

   /* Find var cause I need the number of dims. */
   if ((retval = nc4_find_g_var_nc(nc, ncid, varid, &grp, &var)))
      return retval;

   /* Allocate space for the size_t copy of the chunksizes array. */
   if (var->ndims)
      if (!(cs = malloc(var->ndims * sizeof(size_t))))
         return NC_ENOMEM;

   retval = NC4_inq_var_all(ncid, varid, NULL, NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL, contiguousp, cs, NULL,
                            NULL, NULL, NULL, NULL, NULL);

   /* Copy from size_t array. */
   if (chunksizesp && var->contiguous == NC_CHUNKED)
      for (i = 0; i < var->ndims; i++)
      {
         chunksizesp[i] = (int)cs[i];
         if (cs[i] > NC_MAX_INT)
            retval = NC_ERANGE;
      }

   if (var->ndims)
      free(cs);
   return retval;
}

/**
 * @internal Define chunking stuff for a var. This is called by
 * the fortran API.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param contiguous Pointer to contiguous setting.
 * @param chunksizesp Array of chunksizes.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EINVAL Invalid input
 * @returns ::NC_EBADCHUNK Bad chunksize.
 * @author Ed Hartnett
 */
int
nc_def_var_chunking_ints(int ncid, int varid, int contiguous, int *chunksizesp)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_VAR_INFO_T *var;
   NC_HDF5_FILE_INFO_T *h5;
   size_t *cs = NULL;
   int i, retval;

   /* Find this ncid's file info. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(nc);

   /* Find var cause I need the number of dims. */
   if ((retval = nc4_find_g_var_nc(nc, ncid, varid, &grp, &var)))
      return retval;

   /* Allocate space for the size_t copy of the chunksizes array. */
   if (var->ndims)
      if (!(cs = malloc(var->ndims * sizeof(size_t))))
         return NC_ENOMEM;

   /* Copy to size_t array. */
   for (i = 0; i < var->ndims; i++)
      cs[i] = chunksizesp[i];

   retval = nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL,
                             &contiguous, cs, NULL, NULL, NULL);

   if (var->ndims)
      free(cs);
   return retval;
}

/**
 * @internal This functions sets fill value and no_fill mode for a
 * netCDF-4 variable. It is called by nc_def_var_fill().
 *
 * @note All pointer parameters may be NULL, in which case they are ignored.
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param no_fill No_fill setting.
 * @param fill_value Pointer to fill value.
 *
 * @returns ::NC_NOERR for success
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
 * netcdf-4 file.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EPERM File is read only.
 * @returns ::NC_EINVAL Invalid input
 * @author Ed Hartnett
 */
int
NC4_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL, NULL,
                           NULL, &no_fill, fill_value, NULL);
}

/**
 * @internal This functions sets endianness for a netCDF-4
 * variable. Called by nc_def_var_endian().
 *
 * @note All pointer parameters may be NULL, in which case they are ignored.
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param endianness Endianness setting.
 *
 * @returns ::NC_NOERR for success
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
 * netcdf-4 file.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_ENOTINDEFINE Not in define mode.
 * @returns ::NC_EPERM File is read only.
 * @returns ::NC_EINVAL Invalid input
 * @author Ed Hartnett
 */
int
NC4_def_var_endian(int ncid, int varid, int endianness)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL, NULL,
                           NULL, NULL, NULL, &endianness);
}

/**
 * @internal Define filter settings. Called by nc_def_var_filter().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param id Filter ID
 * @param nparams Number of parameters for filter.
 * @param parms Filter parameters.
 *
 * @returns ::NC_NOERR for success
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_EFILTER Filter error.
 * @returns ::NC_EINVAL Invalid input
 * @author Dennis Heimbigner
 */
int
NC4_def_var_filter(int ncid, int varid, unsigned int id, size_t nparams,
                   const unsigned int* parms)
{
   int retval = NC_NOERR;
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;

   LOG((2, "%s: ncid 0x%x varid %d", __func__, ncid, varid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(nc && grp && h5);

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

   /* Can't turn on parallel and filters */
   if (nc->mode & (NC_MPIIO | NC_MPIPOSIX)) {
      return NC_EINVAL;
   }

   /* If the HDF5 dataset has already been created, then it is too
    * late to set all the extra stuff. */
   if (var->created)
      return NC_ELATEDEF;

#ifdef HAVE_H5Z_SZIP
   if(id == H5Z_FILTER_SZIP) {
      if(nparams != 2)
         return NC_EFILTER; /* incorrect no. of parameters */
   }
#else /*!HAVE_H5Z_SZIP*/
   if(id == H5Z_FILTER_SZIP)
      return NC_EFILTER; /* Not allowed */
#endif

#if 0
   {
      unsigned int fcfg = 0;
      herr_t herr = H5Zget_filter_info(id,&fcfg);
      if(herr < 0)
         return NC_EFILTER;
      if((H5Z_FILTER_CONFIG_ENCODE_ENABLED & fcfg) == 0
         || (H5Z_FILTER_CONFIG_DECODE_ENABLED & fcfg) == 0)
         return NC_EFILTER;
   }
#endif /*0*/

   var->filterid = id;
   var->nparams = nparams;
   var->params = NULL;
   if(parms != NULL) {
      var->params = (unsigned int*)calloc(nparams,sizeof(unsigned int));
      if(var->params == NULL) return NC_ENOMEM;
      memcpy(var->params,parms,sizeof(unsigned int)*var->nparams);
   }
   return NC_NOERR;
}

/**
 * @internal Find the ID of a variable, from the name. This function
 * is called by nc_inq_varid().
 *
 * @param ncid File ID.
 * @param name Name of the variable.
 * @param varidp Gets variable ID.

 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Bad variable ID.
 */
int
NC4_inq_varid(int ncid, const char *name, int *varidp)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_VAR_INFO_T *var;
   char norm_name[NC_MAX_NAME + 1];
   int retval;
   uint32_t nn_hash;
   int i;

   if (!name)
      return NC_EINVAL;
   if (!varidp)
      return NC_NOERR;

   LOG((2, "%s: ncid 0x%x name %s", __func__, ncid, name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, NULL)))
      return retval;

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   nn_hash = hash_fast(norm_name, strlen(norm_name));

   /* Find var of this name. */
   for (i=0; i < grp->vars.nelems; i++)
   {
      var = grp->vars.value[i];
      if (!var) continue;
      if (nn_hash == var->hash && !(strcmp(var->name, norm_name)))
      {
         *varidp = var->varid;
         return NC_NOERR;
      }
   }
   return NC_ENOTVAR;
}

/**
 * @internal Rename a var to "bubba," for example. This is called by
 * nc_rename_var() for netCDF-4 files.
 *
 * @param ncid File ID.
 * @param varid Variable ID
 * @param name New name of the variable.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_EBADNAME Bad name.
 * @returns ::NC_EMAXNAME Name is too long.
 * @returns ::NC_ENAMEINUSE Name in use.
 * @returns ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
*/
int
NC4_rename_var(int ncid, int varid, const char *name)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var, *tmp_var;
   uint32_t nn_hash;
   int retval = NC_NOERR;
   int i;

   if (!name)
      return NC_EINVAL;

   LOG((2, "%s: ncid 0x%x varid %d name %s", __func__, ncid, varid,
        name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;
   assert(h5 && grp && h5);

   /* Is the new name too long? */
   if (strlen(name) > NC_MAX_NAME)
      return NC_EMAXNAME;

   /* Trying to write to a read-only file? No way, Jose! */
   if (h5->no_write)
      return NC_EPERM;

   /* Check name validity, if strict nc3 rules are in effect for this
    * file. */
   if ((retval = NC_check_name(name)))
      return retval;

   /* Check if name is in use, and retain a pointer to the correct variable */
   nn_hash = hash_fast(name, strlen(name));
   tmp_var = NULL;
   for (i=0; i < grp->vars.nelems; i++)
   {
      var = grp->vars.value[i];
      if (!var) continue;
      if (nn_hash == var->hash && !strncmp(var->name, name, NC_MAX_NAME))
         return NC_ENAMEINUSE;
      if (var->varid == varid)
         tmp_var = var;
   }
   if (!tmp_var)
      return NC_ENOTVAR;
   var = tmp_var;

   /* If we're not in define mode, new name must be of equal or
      less size, if strict nc3 rules are in effect for this . */
   if (!(h5->flags & NC_INDEF) && strlen(name) > strlen(var->name) &&
       (h5->cmode & NC_CLASSIC_MODEL))
      return NC_ENOTINDEFINE;

   /* Change the HDF5 file, if this var has already been created
      there. Should we check here to ensure there is not already a
      dimscale dataset of name name??? */
   if (var->created)
   {
      /* Is there an existing dimscale-only dataset of this name? If
       * so, it must be deleted. */
      if (var->ndims && var->dim[0]->hdf_dimscaleid)
      {
         if ((retval = delete_existing_dimscale_dataset(grp, var->dim[0]->dimid, var->dim[0])))
            return retval;
      }
      
      LOG((3, "Moving dataset %s to %s", var->name, name));
      if (H5Gmove(grp->hdf_grpid, var->name, name) < 0)
         BAIL(NC_EHDFERR);
   }

   /* Now change the name in our metadata. */
   free(var->name);
   if (!(var->name = malloc((strlen(name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(var->name, name);
   var->hash = nn_hash;
   LOG((3, "var is now %s", var->name));

   /* Check if this was a coordinate variable previously, but names are different now */
   if (var->dimscale && strcmp(var->name, var->dim[0]->name))
   {
      /* Break up the coordinate variable */
      if ((retval = nc4_break_coord_var(grp, var, var->dim[0])))
         return retval;
   }

   /* Check if this should become a coordinate variable */
   if (!var->dimscale)
   {
      /* Only variables with >0 dimensions can become coordinate variables */
      if (var->ndims)
      {
         NC_GRP_INFO_T *dim_grp;
         NC_DIM_INFO_T *dim;

         /* Check to see if this is became a coordinate variable.  If so, it
          * will have the same name as dimension index 0. If it is a
          * coordinate var, is it a coordinate var in the same group as the dim?
          */
         if ((retval = nc4_find_dim(grp, var->dimids[0], &dim, &dim_grp)))
            return retval;
         if (!strcmp(dim->name, name) && dim_grp == grp)
         {
            /* Reform the coordinate variable */
            if ((retval = nc4_reform_coord_var(grp, var, dim)))
               return retval;
            var->became_coord_var = NC_TRUE;
         }
      }
   }

exit:
   return retval;
}

/**
 * @internal
 *
 * This function will change the parallel access of a variable from
 * independent to collective.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param par_access NC_COLLECTIVE or NC_INDEPENDENT.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Invalid ncid passed.
 * @returns ::NC_ENOTVAR Invalid varid passed.
 * @returns ::NC_ENOPAR LFile was not opened with nc_open_par/nc_create_var.
 * @returns ::NC_EINVAL Invalid par_access specified.
 * @returns ::NC_NOERR for success
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_var_par_access(int ncid, int varid, int par_access)
{
#ifndef USE_PARALLEL4
   return NC_ENOPAR;
#else
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   int retval;

   LOG((1, "%s: ncid 0x%x varid %d par_access %d", __func__, ncid,
        varid, par_access));

   if (par_access != NC_INDEPENDENT && par_access != NC_COLLECTIVE)
      return NC_EINVAL;

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   /* This function only for files opened with nc_open_par or nc_create_par. */
   if (!h5->parallel)
      return NC_ENOPAR;

   /* Find the var, and set its preference. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

   if (par_access)
      var->parallel_access = NC_COLLECTIVE;
   else
      var->parallel_access = NC_INDEPENDENT;
   return NC_NOERR;
#endif /* USE_PARALLEL4 */
}

/**
 * @internal Write an array of data to a variable. This is called by
 * nc_put_vara() and other nc_put_vara_* functions, for netCDF-4
 * files.
 * 
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Array of start indicies.
 * @param countp Array of counts.
 * @param op pointer that gets the data.
 * @param memtype The type of these data in memory.
 *
 * @returns ::NC_NOERR for success
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_put_vara(int ncid, int varid, const size_t *startp,
             const size_t *countp, const void *op, int memtype)
{
   NC *nc;

   if (!(nc = nc4_find_nc_file(ncid, NULL)))
      return NC_EBADID;

   return nc4_put_vara(nc, ncid, varid, startp, countp, memtype, 0, (void *)op);
}

/**
 * Read an array of values. This is called by nc_get_vara() for
 * netCDF-4 files, as well as all the other nc_get_vara_*
 * functions.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Array of start indicies.
 * @param countp Array of counts.
 * @param ip pointer that gets the data.
 * @param memtype The type of these data after it is read into memory.

 * @returns ::NC_NOERR for success
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_get_vara(int ncid, int varid, const size_t *startp,
             const size_t *countp, void *ip, int memtype)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T* h5;

   LOG((2, "%s: ncid 0x%x varid %d memtype %d", __func__, ncid, varid,
        memtype));

   if (!(nc = nc4_find_nc_file(ncid, &h5)))
      return NC_EBADID;

   /* Get the data. */
   return nc4_get_vara(nc, ncid, varid, startp, countp, memtype,
                       0, (void *)ip);
}
