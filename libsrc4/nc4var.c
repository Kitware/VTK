/*
This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc4 variable functions.

Copyright 2003-2006, University Corporation for Atmospheric
Research. See COPYRIGHT file for copying and redistribution
conditions.
*/

#include <nc4internal.h>
#include "nc4dispatch.h"
#include <math.h>

/* Min and max deflate levels tolerated by HDF5. */
#define MIN_DEFLATE_LEVEL 0
#define MAX_DEFLATE_LEVEL 9

/* This is to track opened HDF5 objects to make sure they are
 * closed. */
#ifdef EXTRA_TESTS
extern int num_plists;
#endif /* EXTRA_TESTS */

/* One meg is the minimum buffer size. */
#define ONE_MEG 1048576

/* Szip options. */
#define NC_SZIP_EC_OPTION_MASK 4
#define NC_SZIP_NN_OPTION_MASK 32
#define NC_SZIP_MAX_PIXELS_PER_BLOCK 32

extern int nc4_get_default_fill_value(const NC_TYPE_INFO_T *type_info, void *fill_value);


/* If the HDF5 dataset for this variable is open, then close it and
 * reopen it, with the perhaps new settings for chunk caching. */
int
nc4_reopen_dataset(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
   hid_t access_pid;

   if (var->hdf_datasetid)
   {
      if ((access_pid = H5Pcreate(H5P_DATASET_ACCESS)) < 0)
	 return NC_EHDFERR;
#ifdef EXTRA_TESTS
      num_plists++;
#endif
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
#ifdef EXTRA_TESTS
      num_plists--;
#endif
   }

   return NC_NOERR;
}

/* Set chunk cache size for a variable. */
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

   /* An attempt to do any of these things on a netCDF-3 file is
    * ignored with no error. */
   if (!h5)
      return NC_NOERR;

   assert(nc && grp && h5);

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
     return NC_ENOTVAR;
   var = grp->vars.value[varid];
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

   /* Set the values. */
   var->chunk_cache_size = size;
   var->chunk_cache_nelems = nelems;
   var->chunk_cache_preemption = preemption;

   if ((retval = nc4_reopen_dataset(grp, var)))
      return retval;

   return NC_NOERR;
}

/* Need this version for fortran. Accept negative numbers to leave
 * settings as they are. */
int
nc_set_var_chunk_cache_ints(int ncid, int varid, int size, int nelems,
			    int preemption)
{
   size_t real_size = H5D_CHUNK_CACHE_NBYTES_DEFAULT;
   size_t real_nelems = H5D_CHUNK_CACHE_NSLOTS_DEFAULT;
   float real_preemption = H5D_CHUNK_CACHE_W0_DEFAULT;

   if (size >= 0)
       real_size = ((size_t) size) * MEGABYTE;

   if (nelems >= 0)
      real_nelems = nelems;

   if (preemption >= 0)
      real_preemption = preemption / 100.;

   return NC4_set_var_chunk_cache(ncid, varid, real_size, real_nelems,
				 real_preemption);
}

/* Get chunk cache size for a variable. */
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

   /* Attempting to do any of these things on a netCDF-3 file produces
    * an error. */
   if (!h5)
      return NC_ENOTNC4;

   assert(nc && grp && h5);

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
     return NC_ENOTVAR;
   var = grp->vars.value[varid];
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

   /* Give the user what they want. */
   if (sizep)
      *sizep = var->chunk_cache_size;
   if (nelemsp)
      *nelemsp = var->chunk_cache_nelems;
   if (preemptionp)
      *preemptionp = var->chunk_cache_preemption;

   return NC_NOERR;
}

/* Get chunk cache size for a variable. */
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

/* Check a set of chunksizes to see if they specify a chunk that is too big. */
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
   {
      if (chunksizes[d] < 1)
	 return NC_EINVAL;
      dprod *= (double) chunksizes[d];
   }

   if (dprod > (double) NC_MAX_UINT)
      return NC_EBADCHUNK;

   return NC_NOERR;
}

/* Find the default chunk nelems (i.e. length of chunk along each
 * dimension). */
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
#define DEFAULT_1D_UNLIM_SIZE (4096) /* TODO: make build-time parameter? */
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

#define NC_ARRAY_GROWBY 4
int nc4_vararray_add(NC_GRP_INFO_T *grp,
		     NC_VAR_INFO_T *var)
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

  if(var != NULL) {
    assert(var->varid == grp->vars.nelems);
    grp->vars.value[grp->vars.nelems] = var;
    grp->vars.nelems++;
  }
  return NC_NOERR;
}

/* This is called when a new netCDF-4 variable is defined. Break it
 * down! */
static int
nc_def_var_nc4(int ncid, const char *name, nc_type xtype,
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
    * otherwise switch to define mode. */
   if (!(h5->flags & NC_INDEF))
   {
      if (h5->cmode & NC_CLASSIC_MODEL)
         BAIL(NC_ENOTINDEFINE);
      if ((retval = NC4_redef(ncid)))
         BAIL(retval);
   }

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      BAIL(retval);

   /* Not a Type is, well, not a type.*/
   if (xtype == NC_NAT)
      BAIL(NC_EBADTYPE);

   /* For classic files, only classic types are allowed. */
   if (h5->cmode & NC_CLASSIC_MODEL && xtype > NC_DOUBLE)
      BAIL(NC_ESTRICTNC3);

   /* cast needed for braindead systems with signed size_t */
   if((unsigned long) ndims > X_INT_MAX) /* Backward compat */
      BAIL(NC_EINVAL);

   /* Check that this name is not in use as a var, grp, or type. */
   if ((retval = nc4_check_dup_name(grp, norm_name)))
      BAIL(retval);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
     BAIL(NC_EPERM);

   /* Check all the dimids to make sure they exist. */
   for (d = 0; d < ndims; d++)
      if ((retval = nc4_find_dim(grp, dimidsp[d], &dim, NULL)))
         BAIL(retval);

   /* These degrubbing messages sure are handy! */
   LOG((3, "%s: name %s type %d ndims %d", __func__, norm_name, xtype, ndims));
#ifdef LOGGING
   {
      int dd;
      for (dd = 0; dd < ndims; dd++)
         LOG((4, "dimid[%d] %d", dd, dimidsp[dd]));
   }
#endif

   /* Add a new var. */
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

   nc4_vararray_add(grp, var);

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

         /* Use variable's dataset ID for the dimscale ID */
         if (dim->hdf_dimscaleid)
         {
            /* Detach dimscale from any variables using it */
            if ((retval = rec_detach_scales(grp, dimidsp[d], dim->hdf_dimscaleid)) < 0)
               BAIL(retval);

            if (H5Dclose(dim->hdf_dimscaleid) < 0)
                BAIL(NC_EHDFERR);
            dim->hdf_dimscaleid = 0;

            /* Now delete the dataset (it will be recreated later, if necessary) */
            if (H5Gunlink(grp->hdf_grpid, dim->name) < 0)
               BAIL(NC_EDIMMETA);
         }
      }

      /* Check for unlimited dimension and turn off contiguous storage */
      /* (unless HDF4 file) */
#ifdef USE_HDF4
      if (dim->unlimited && !h5->hdf4)
#else
      if (dim->unlimited)
#endif
	 var->contiguous = NC_FALSE;

      /* Track dimensions for variable */
      var->dimids[d] = dimidsp[d];
      var->dim[d] = dim;
   }

   /* Determine default chunksizes for this variable. (Even for
    * variables which may be contiguous. */
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
      if (ndims && !(var->dimscale_attached = calloc(ndims, sizeof(nc_bool_t))))
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

/* Create a new variable to hold user data. This is what it's all
 * about baby! */
int
NC4_def_var(int ncid, const char *name, nc_type xtype, int ndims,
           const int *dimidsp, int *varidp)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;

   LOG((2, "%s: ncid 0x%x name %s xtype %d ndims %d",
        __func__, ncid, name, xtype, ndims));

   /* If there are dimensions, I need their ids. */
   if (ndims && !dimidsp)
      return NC_EINVAL;

   /* Find metadata for this file. */
   if (!(nc = nc4_find_nc_file(ncid,&h5)))
      return NC_EBADID;

   /* Handle netcdf-4 cases. */
   return nc_def_var_nc4(ncid, name, xtype, ndims, dimidsp, varidp);
}

/* Get all the information about a variable. Pass NULL for whatever
 * you don't care about. This is an internal function, not exposed to
 * the user. */
int
NC4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
               int *ndimsp, int *dimidsp, int *nattsp,
               int *shufflep, int *deflatep, int *deflate_levelp,
               int *fletcher32p, int *contiguousp, size_t *chunksizesp,
               int *no_fill, void *fill_valuep, int *endiannessp,
	       int *options_maskp, int *pixels_per_blockp)
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
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

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
   /* NOTE: No interface for returning szip flag currently (but it should never
    *   be set).
    */
   if (options_maskp)
      *options_maskp = var->options_mask;
   if (pixels_per_blockp)
      *pixels_per_blockp = var->pixels_per_block;

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
             if (*(char **)var->fill_value) {

               if (!(fill_valuep = calloc(1, sizeof(char *))))
                 return NC_ENOMEM;

               if (!(*(char **)fill_valuep = strdup(*(char **)var->fill_value)))
                 {
                   free(fill_valuep);
                   return NC_ENOMEM;
                 }
             }
           }
         else {
             assert(var->type_info->size);
             memcpy(fill_valuep, var->fill_value, var->type_info->size);
         }
      }
      else
      {
         if (var->type_info->nc_type_class == NC_STRING)
         {
            if (!(fill_valuep = calloc(1, sizeof(char *))))
               return NC_ENOMEM;

            if ((retval = nc4_get_default_fill_value(var->type_info, (char **)fill_valuep)))
            {
               free(fill_valuep);
               return retval;
            } else {
	      free(fill_valuep);
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

/* This functions sets extra stuff about a netCDF-4 variable which
   must be set before the enddef but after the def_var. This is an
   internal function, deliberately hidden from the user so that we can
   change the prototype of this functions without changing the API. */
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
   NC_DIM_INFO_T *dim;
   int d;
   int retval;
   nc_bool_t ishdf4 = NC_FALSE; /* Use this to avoid so many ifdefs */

   LOG((2, "%s: ncid 0x%x varid %d", __func__, ncid, varid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

#ifdef USE_HDF4
   ishdf4 = h5->hdf4;
#endif

   /* Attempting to do any of these things on a netCDF-3 file produces
    * an error. */
   if (!h5)
      return NC_ENOTNC4;

   assert(nc && grp && h5);

   /* Find the var. */
   if (varid < 0 || varid >= grp->vars.nelems)
     return NC_ENOTVAR;
   var = grp->vars.value[varid];
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

   /* Can't turn on contiguous and deflate/fletcher32/szip. */
   if (contiguous)
      if ((*contiguous != NC_CHUNKED && deflate) ||
	  (*contiguous != NC_CHUNKED && fletcher32))
	 return NC_EINVAL;

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
   if (deflate && deflate_level)
   {
      if (*deflate)
         if (*deflate_level < MIN_DEFLATE_LEVEL ||
             *deflate_level > MAX_DEFLATE_LEVEL)
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

     if (!ishdf4) {
      for (d = 0; d < var->ndims; d++)
      {
	 dim = var->dim[d];
	 if (dim->unlimited)
	    return NC_EINVAL;
      }
      var->contiguous = NC_TRUE;
    }
   }

   /* Chunksizes anyone? */
   if (!ishdf4 && contiguous && *contiguous == NC_CHUNKED)
   {
      var->contiguous = NC_FALSE;

      /* If the user provided chunksizes, check that they are not too
       * big, and that their total size of chunk is less than 4 GB. */
      if (chunksizes)
      {

	 if ((retval = check_chunksizes(grp, var, chunksizes)))
	    return retval;
	 for (d = 0; d < var->ndims; d++) {
	    if(var->dim[d]->len > 0 && chunksizes[d] > var->dim[d]->len)
	       return NC_EBADCHUNK;
	 }

	 /* Set the chunksizes for this variable. */
	 for (d = 0; d < var->ndims; d++)
	    var->chunksizes[d] = chunksizes[d];
      }
   }

   /* Is this a variable with a chunksize greater than the current
    * cache size? */
   if (!var->contiguous && (chunksizes || deflate || contiguous))
   {
      /* Determine default chunksizes for this variable. */
      if (!var->chunksizes[0])
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
         var->no_fill = NC_TRUE;
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

/* Set the deflate level for a var, lower is faster, higher is
 * better. Must be called after nc_def_var and before nc_enddef or any
 * functions which writes data to the file. */
int
NC4_def_var_deflate(int ncid, int varid, int shuffle, int deflate,
                   int deflate_level)
{
   return nc_def_var_extra(ncid, varid, &shuffle, &deflate,
                           &deflate_level, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Set checksum for a var. This must be called after the nc_def_var
 * but before the nc_enddef. */
int
NC4_def_var_fletcher32(int ncid, int varid, int fletcher32)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, &fletcher32,
                           NULL, NULL, NULL, NULL, NULL);
}

/* Define chunking stuff for a var. This must be done after nc_def_var
   and before nc_enddef.

   Chunking is required in any dataset with one or more unlimited
   dimensions in HDF5, or any dataset using a filter.

   Where chunksize is a pointer to an array of size ndims, with the
   chunksize in each dimension.
*/
int
NC4_def_var_chunking(int ncid, int varid, int contiguous, const size_t *chunksizesp)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL,
                           &contiguous, chunksizesp, NULL, NULL, NULL);
}

/* Inquire about chunking stuff for a var. This is a private,
 * undocumented function, used by the f77 API to avoid size_t
 * problems. */
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
                           NULL, NULL, NULL, NULL);

   /* Copy from size_t array. */
   if (*contiguousp == NC_CHUNKED)
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

/* This function defines the chunking with ints, which works better
 * with F77 portability. It is a secret function, which has been
 * rendered unmappable, and it is impossible to apparate anywhere in
 * this function. */
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

#ifdef USE_HDF4
   if(h5->hdf4)
	return NC_NOERR;
#endif

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

/* Define fill value behavior for a variable. This must be done after
   nc_def_var and before nc_enddef. */
int
NC4_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL, NULL,
                           NULL, &no_fill, fill_value, NULL);
}


/* Define the endianness of a variable. */
int
NC4_def_var_endian(int ncid, int varid, int endianness)
{
   return nc_def_var_extra(ncid, varid, NULL, NULL, NULL, NULL, NULL,
                           NULL, NULL, NULL, &endianness);
}

/* Get var id from name. */
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

/* Rename a var to "bubba," for example.

   According to the netcdf-3.5 docs: If the new name is longer than
   the old name, the netCDF dataset must be in define mode.  */
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

   LOG((2, "%s: ncid 0x%x varid %d name %s",
        __func__, ncid, varid, name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(h5);

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
      there. */
   if (var->created)
   {
      if (H5Gmove(grp->hdf_grpid, var->name, name) < 0)
         BAIL(NC_EHDFERR);
   }

   /* Now change the name in our metadata. */
   free(var->name);
   if (!(var->name = malloc((strlen(name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(var->name, name);
   var->hash = nn_hash;

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
         if (strcmp(dim->name, name) == 0 && dim_grp == grp)
         {
             /* Reform the coordinate variable */
             if ((retval = nc4_reform_coord_var(grp, var, dim)))
                return retval;
         }
      }
   }

  exit:
   return retval;
}


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

static int
nc4_put_vara_tc(int ncid, int varid, nc_type mem_type, int mem_type_is_long,
                const size_t *startp, const size_t *countp, const void *op)
{
   NC *nc;

   LOG((2, "%s: ncid 0x%x varid %d mem_type %d mem_type_is_long %d",
        __func__, ncid, varid, mem_type, mem_type_is_long));

   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   return nc4_put_vara(nc, ncid, varid, startp, countp, mem_type,
                       mem_type_is_long, (void *)op);
}

#ifdef USE_HDF4
static int
nc4_get_hdf4_vara(NC *nc, int ncid, int varid, const size_t *startp,
		  const size_t *countp, nc_type mem_nc_type, int is_long, void *data)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   int32 start32[NC_MAX_VAR_DIMS], edge32[NC_MAX_VAR_DIMS];
   int retval, d;

   /* Find our metadata for this file, group, and var. */
   assert(nc);
   if ((retval = nc4_find_g_var_nc(nc, ncid, varid, &grp, &var)))
      return retval;
   h5 = NC4_DATA(nc);
   assert(grp && h5 && var && var->name);

   for (d = 0; d < var->ndims; d++)
   {
      start32[d] = startp[d];
      edge32[d] = countp[d];
   }

   if (SDreaddata(var->sdsid, start32, NULL, edge32, data))
      return NC_EHDFERR;

   return NC_NOERR;
}
#endif /* USE_HDF4 */

/* Get an array. */
static int
nc4_get_vara_tc(int ncid, int varid, nc_type mem_type, int mem_type_is_long,
                const size_t *startp, const size_t *countp, void *ip)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T* h5;

   LOG((2, "%s: ncid 0x%x varid %d mem_type %d mem_type_is_long %d",
        __func__, ncid, varid, mem_type, mem_type_is_long));

   if (!(nc = nc4_find_nc_file(ncid,&h5)))
      return NC_EBADID;

#ifdef USE_HDF4
   /* Handle HDF4 cases. */
   if (h5->hdf4)
      return nc4_get_hdf4_vara(nc, ncid, varid, startp, countp, mem_type,
			       mem_type_is_long, (void *)ip);
#endif /* USE_HDF4 */

   /* Handle HDF5 cases. */
   return nc4_get_vara(nc, ncid, varid, startp, countp, mem_type,
                       mem_type_is_long, (void *)ip);
}

int
NC4_put_vara(int ncid, int varid, const size_t *startp,
            const size_t *countp, const void *op, int memtype)
{
   return nc4_put_vara_tc(ncid, varid, memtype, 0, startp, countp, op);
}


/* Read an array of values. */
int
NC4_get_vara(int ncid, int varid, const size_t *startp,
            const size_t *countp, void *ip, int memtype)
{
   return nc4_get_vara_tc(ncid, varid, memtype, 0, startp, countp, ip);
}

void
nc4verify(int ncid, char* name)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   int retval;

   /* Find info for this file and group, and set pointer to each. */
   retval = nc4_find_grp_h5(ncid, &grp, &h5);
   assert(grp && h5);
   retval = nc4_check_dup_name(grp, name);
   return;
}
