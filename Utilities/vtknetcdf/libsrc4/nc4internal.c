/*
This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file contains functions internal to the netcdf4 library. None of
the functions in this file are exposed in the exetnal API. These
functions all relate to the manipulation of netcdf-4's in-memory
buffer of metadata information, i.e. the linked list of NC_FILE_INFO_T
structs.

Copyright 2003-2005, University Corporation for Atmospheric
Research. See the COPYRIGHT file for copying and redistribution
conditions.

$Id: nc4internal.c,v 1.121 2010/05/26 21:43:35 dmh Exp $
*/

#include "ncconfig.h"
#include "nc4internal.h"
#include "nc.h" /* from libsrc */
#include "ncdispatch.h" /* from libdispatch */
#include <utf8proc.h>

#define MEGABYTE 1048576

/* These are the default chunk cache sizes for HDF5 files created or
 * opened with netCDF-4. */
extern size_t nc4_chunk_cache_size;
extern size_t nc4_chunk_cache_nelems;
extern float nc4_chunk_cache_preemption;

/* This is to track opened HDF5 objects to make sure they are
 * closed. */
#ifdef EXTRA_TESTS
extern int num_spaces;
#endif /* EXTRA_TESTS */

#ifdef LOGGING
/* This is the severity level of messages which will be logged. Use
   severity 0 for errors, 1 for important log messages, 2 for less
   important, etc. */
int nc_log_level = -1;

#endif /* LOGGING */

/* Check and normalize and name. */
int
nc4_check_name(const char *name, char *norm_name)
{
   char *temp;
   int retval;

   /* Check the length. */
   if (strlen(name) > NC_MAX_NAME)
      return NC_EMAXNAME;

   /* Make sure this is a valid netcdf name. This should be done
    * before the name is normalized, because it gives better error
    * codes for bad utf8 strings. */
   if ((retval = NC_check_name(name)))
      return retval;

   /* Normalize the name. */
   if (!(temp = (char *)utf8proc_NFC((const unsigned char *)name)))
      return NC_EINVAL;
   strcpy(norm_name, temp);
   free(temp);

   return NC_NOERR;
}

/* Given a varid, find its shape. For unlimited dimensions, return
   the current number of records. */
static int
find_var_shape_grp(NC_GRP_INFO_T *grp, int varid, int *ndims,
                   int *dimid, size_t *dimlen)
{
   hid_t datasetid = 0, spaceid = 0;
   NC_VAR_INFO_T *var;
   hsize_t *h5dimlen = NULL, *h5dimlenmax = NULL;
   int d, dataset_ndims = 0;
   int retval = NC_NOERR;

   /* Find this var. */
   for (var = grp->var; var; var = var->next)
      if (var->varid == varid)
         break;
   if (!var)
      return NC_ENOTVAR;

   /* Get the dimids and the ndims for this var. */
   if (ndims)
      *ndims = var->ndims;

   if (dimid)
      for (d = 0; d < var->ndims; d++)
         dimid[d] = var->dimids[d];

   if (dimlen)
   {
      /* If the var hasn't been created yet, its size is 0. */
      if (!var->created)
      {
         for (d = 0; d < var->ndims; d++)
            dimlen[d] = 0;
      }
      else
      {
         /* Get the number of records in the dataset. */
         if ((retval = nc4_open_var_grp2(grp, var->varid, &datasetid)))
            BAIL(retval);
         if ((spaceid = H5Dget_space(datasetid)) < 0)
            BAIL(NC_EHDFERR);
#ifdef EXTRA_TESTS
         num_spaces++;
#endif
         /* If it's a scalar dataset, it has length one. */
         if (H5Sget_simple_extent_type(spaceid) == H5S_SCALAR)
         {
            dimlen[0] = 1;
         }
         else
         {
            /* Check to make sure ndims is right, then get the len of each
               dim in the space. */
            if ((dataset_ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
               BAIL(NC_EHDFERR);
            if (dataset_ndims != *ndims)
               BAIL(NC_EHDFERR);
            if (!(h5dimlen = malloc(dataset_ndims * sizeof(hsize_t))))
               BAIL(NC_ENOMEM);
            if (!(h5dimlenmax = malloc(dataset_ndims * sizeof(hsize_t))))
               BAIL(NC_ENOMEM);
            if ((dataset_ndims = H5Sget_simple_extent_dims(spaceid,
                                                           h5dimlen, h5dimlenmax)) < 0)
               BAIL(NC_EHDFERR);
            LOG((5, "find_var_shape_nc: varid %d len %d max: %d",
                 varid, (int)h5dimlen[0], (int)h5dimlenmax[0]));
            for (d=0; d<dataset_ndims; d++)
               dimlen[d] = h5dimlen[d];
         }
      }
   }

  exit:
   if (spaceid > 0 && H5Sclose(spaceid) < 0)
      BAIL2(NC_EHDFERR);
#ifdef EXTRA_TESTS
   num_spaces--;
#endif
   if (h5dimlen) free(h5dimlen);
   if (h5dimlenmax) free(h5dimlenmax);
   return retval;
}

/* Given an NC_FILE_INFO_T pointer, add the necessary stuff for a
 * netcdf-4 file. */
int
nc4_nc4f_list_add(NC_FILE_INFO_T *nc, const char *path, int mode)
{
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;

   assert(nc && !nc->nc4_info && path);

   /* The NC_FILE_INFO_T was allocated and inited by
      ncfunc.c before this function is called. We need to malloc and
      initialize the substructure NC_HDF_FILE_INFO_T. */
   if (!(nc->nc4_info = calloc(1, sizeof(NC_HDF5_FILE_INFO_T))))
      return NC_ENOMEM;
   h5 = nc->nc4_info;

   /* Hang on to the filename for nc_abort. */
   if (!(h5->path = malloc((strlen(path) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(h5->path, path);

   /* Hang on to cmode, and note that we're in define mode. */
   h5->cmode = mode | NC_INDEF;

   /* The next_typeid needs to be set beyond the end of our atomic
    * types. */
   h5->next_typeid = NC_FIRSTUSERTYPEID;

   /* There's always at least one open group - the root
    * group. Allocate space for one group's worth of information. Set
    * its hdf id, name, and a pointer to it's file structure. */
   return nc4_grp_list_add(&(h5->root_grp), h5->next_nc_grpid++,
                           NULL, nc, NC_GROUP_NAME, &grp);
}
/* /\* Given an ncid, find the relevant group and return a pointer to */
/*  * it. *\/ */
/* NC_GRP_INFO_T * */
/* find_nc_grp(int ncid) */
/* { */
/*    NC_FILE_INFO_T *f; */

/*    for (f = nc_file; f; f = f->next) */
/*    { */
/*       if (f->ext_ncid == (ncid & FILE_ID_MASK)) */
/*       { */
/*       assert(f->nc4_info && f->nc4_info->root_grp); */
/*       return nc4_rec_find_grp(f->nc4_info->root_grp, (ncid & GRP_ID_MASK)); */
/*       } */
/*    } */

/*    return NULL; */
/* } */

/* Given an ncid, find the relevant group and return a pointer to it,
 * return an error of this is not a netcdf-4 file (or if strict nc3 is
 * turned on for this file.) */


int
nc4_find_nc4_grp(int ncid, NC_GRP_INFO_T **grp)
{
   NC_FILE_INFO_T *f = nc4_find_nc_file(ncid);
   if(f == NULL) return NC_EBADID;

   /* No netcdf-3 files allowed! */
   if (!f->nc4_info) return NC_ENOTNC4;
   assert(f->nc4_info->root_grp);

   /* This function demands netcdf-4 files without strict nc3
    * rules.*/
   if (f->nc4_info->cmode & NC_CLASSIC_MODEL) return NC_ESTRICTNC3;

   /* If we can't find it, the grp id part of ncid is bad. */
   if (!(*grp = nc4_rec_find_grp(f->nc4_info->root_grp, (ncid & GRP_ID_MASK))))
      return NC_EBADID;
   return NC_NOERR;
}

/* Given an ncid, find the relevant group and return a pointer to it,
 * also set a pointer to the nc4_info struct of the related file. For
 * netcdf-3 files, *h5 will be set to NULL. */
int
nc4_find_grp_h5(int ncid, NC_GRP_INFO_T **grp, NC_HDF5_FILE_INFO_T **h5)
{
    NC_FILE_INFO_T *f = nc4_find_nc_file(ncid);
    if(f == NULL) return NC_EBADID;
    if (f->nc4_info) {
        assert(f->nc4_info->root_grp);
        /* If we can't find it, the grp id part of ncid is bad. */
        if (!(*grp = nc4_rec_find_grp(f->nc4_info->root_grp, (ncid & GRP_ID_MASK))))
            return NC_EBADID;
        *h5 = (*grp)->file->nc4_info;
        assert(*h5);
    } else {
        *h5 = NULL;
        *grp = NULL;
    }
    return NC_NOERR;
}

int
nc4_find_nc_grp_h5(int ncid, NC_FILE_INFO_T **nc, NC_GRP_INFO_T **grp,
                   NC_HDF5_FILE_INFO_T **h5)
{
    NC_FILE_INFO_T *f = nc4_find_nc_file(ncid);
    if(f == NULL) return NC_EBADID;
    *nc = f;
    if (f->nc4_info) {
        assert(f->nc4_info->root_grp);
        /* If we can't find it, the grp id part of ncid is bad. */
        if (!(*grp = nc4_rec_find_grp(f->nc4_info->root_grp, (ncid & GRP_ID_MASK))))
               return NC_EBADID;

        *h5 = (*grp)->file->nc4_info;
        assert(*h5);
    } else {
        *h5 = NULL;
        *grp = NULL;
    }
    return NC_NOERR;
}

/* Recursively hunt for a group id. */
NC_GRP_INFO_T *
nc4_rec_find_grp(NC_GRP_INFO_T *start_grp, int target_nc_grpid)
{
   NC_GRP_INFO_T *g, *res;

   assert(start_grp);

   /* Is this the group we are searching for? */
   if (start_grp->nc_grpid == target_nc_grpid)
      return start_grp;

   /* Shake down the kids. */
   if (start_grp->children)
      for (g = start_grp->children; g; g = g->next)
         if ((res = nc4_rec_find_grp(g, target_nc_grpid)))
            return res;

   /* Can't find if. Fate, why do you mock me? */
   return NULL;
}

/* Given an ncid and varid, get pointers to the group and var
 * metadata. */
int
nc4_find_g_var_nc(NC_FILE_INFO_T *nc, int ncid, int varid,
                  NC_GRP_INFO_T **grp, NC_VAR_INFO_T **var)
{
   /* Find the group info. */
   assert(grp && var && nc && nc->nc4_info && nc->nc4_info->root_grp);
   *grp = nc4_rec_find_grp(nc->nc4_info->root_grp, (ncid & GRP_ID_MASK));

   /* Find the var info. */
   for ((*var) = (*grp)->var; (*var); (*var) = (*var)->next)
     if ((*var)->varid == varid)
       break;
   if (!(*var))
     return NC_ENOTVAR;

   return NC_NOERR;
}

/* Find a dim in a grp (or parents). */
int
nc4_find_dim(NC_GRP_INFO_T *grp, int dimid, NC_DIM_INFO_T **dim,
             NC_GRP_INFO_T **dim_grp)
{
   NC_GRP_INFO_T *g, *dg = NULL;
   int finished = 0;

   assert(grp && dim);

   /* Find the dim info. */
   for (g = grp; g && !finished; g = g->parent)
      for ((*dim) = g->dim; (*dim); (*dim) = (*dim)->next)
         if ((*dim)->dimid == dimid)
         {
            dg = g;
            finished++;
            break;
         }

   /* If we didn't find it, return an error. */
   if (!(*dim))
     return NC_EBADDIM;

   /* Give the caller the group the dimension is in. */
   if (dim_grp)
      *dim_grp = dg;

   return NC_NOERR;
}

/* Recursively hunt for a HDF type id. */
NC_TYPE_INFO_T *
nc4_rec_find_hdf_type(NC_GRP_INFO_T *start_grp, hid_t target_hdf_typeid)
{
   NC_GRP_INFO_T *g;
   NC_TYPE_INFO_T *type, *res;
   htri_t equal;

   assert(start_grp);

   /* Does this group have the type we are searching for? */
   for (type = start_grp->type; type; type = type->next)
   {
      if ((equal = H5Tequal(type->native_typeid ? type->native_typeid : type->hdf_typeid, target_hdf_typeid)) < 0)
         return NULL;
      if (equal)
         return type;
   }

   /* Shake down the kids. */
   if (start_grp->children)
      for (g = start_grp->children; g; g = g->next)
         if ((res = nc4_rec_find_hdf_type(g, target_hdf_typeid)))
            return res;

   /* Can't find if. Fate, why do you mock me? */
   return NULL;
}

/* Recursively hunt for a netCDF type id. */
NC_TYPE_INFO_T *
nc4_rec_find_nc_type(NC_GRP_INFO_T *start_grp, nc_type target_nc_typeid)
{
   NC_GRP_INFO_T *g;
   NC_TYPE_INFO_T *type, *res;

   assert(start_grp);

   /* Does this group have the type we are searching for? */
   for (type = start_grp->type; type; type = type->next)
      if (type->nc_typeid == target_nc_typeid)
         return type;

   /* Shake down the kids. */
   if (start_grp->children)
      for (g = start_grp->children; g; g = g->next)
         if ((res = nc4_rec_find_nc_type(g, target_nc_typeid)))
            return res;

   /* Can't find if. Fate, why do you mock me? */
   return NULL;
}

/* Recursively hunt for a netCDF type by name. */
NC_TYPE_INFO_T *
nc4_rec_find_named_type(NC_GRP_INFO_T *start_grp, char *name)
{
   NC_GRP_INFO_T *g;
   NC_TYPE_INFO_T *type, *res;

   assert(start_grp);

   /* Does this group have the type we are searching for? */
   for (type = start_grp->type; type; type = type->next)
      if (!strcmp(type->name, name))
         return type;

   /* Search subgroups. */
   if (start_grp->children)
      for (g = start_grp->children; g; g = g->next)
         if ((res = nc4_rec_find_named_type(g, name)))
            return res;

   /* Can't find if. Oh, woe is me! */
   return NULL;
}

/* Use a netCDF typeid to find a type in a type_list. */
int
nc4_find_type(NC_HDF5_FILE_INFO_T *h5, nc_type typeid, NC_TYPE_INFO_T **type)
{
   if (typeid < 0 || !type)
      return NC_EINVAL;
   *type = NULL;

   /* Atomic types don't have associated NC_TYPE_INFO_T struct, just
    * return NOERR. */
   if (typeid <= NC_STRING)
      return NC_NOERR;

   /* Find the type. */
   if(!(*type = nc4_rec_find_nc_type(h5->root_grp, typeid)))
      return NC_EBADTYPID;

   return NC_NOERR;
}

/* Find the actual length of a dim by checking the length of that dim
 * in all variables that use it, in grp or children. *len must be
 * initialized to zero before this function is called. */
int
nc4_find_dim_len(NC_GRP_INFO_T *grp, int dimid, size_t **len)
{
   NC_GRP_INFO_T *g;
   NC_VAR_INFO_T *var;
   int d, ndims, dimids[NC_MAX_DIMS];
   size_t dimlen[NC_MAX_DIMS];
   int retval;

   assert(grp && len);
   LOG((3, "nc4_find_dim_len: grp->name %s dimid %d", grp->name, dimid));

   /* If there are any groups, call this function recursively on
    * them. */
   for (g = grp->children; g; g = g->next)
      if ((retval = nc4_find_dim_len(g, dimid, len)))
         return retval;

   /* For all variables in this group, find the ones that use this
    * dimension, and remember the max length. */
   for (var = grp->var; var; var = var->next)
   {
      /* Find dimensions of this var. */
      if ((retval = find_var_shape_grp(grp, var->varid, &ndims,
                                       dimids, dimlen)))
         return retval;

      /* Check for any dimension that matches dimid. If found, check
       * if its length is longer than *lenp. */
      for (d = 0; d < ndims; d++)
      {
         if (dimids[d] == dimid)
         {
            /* Remember the max length in *lenp. */
            **len = dimlen[d] > **len ? dimlen[d] : **len;
            break;
         }
      }
   }

   return NC_NOERR;
}

/* Given a group, find an att. */
int
nc4_find_grp_att(NC_GRP_INFO_T *grp, int varid, const char *name, int attnum,
                 NC_ATT_INFO_T **att)
{
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T *attlist = NULL;

   assert(grp && grp->name);
   LOG((4, "nc4_find_grp_att: grp->name %s varid %d name %s attnum %d",
        grp->name, varid, name, attnum));

   /* Get either the global or a variable attribute list. */
   if (varid == NC_GLOBAL)
      attlist = grp->att;
   else
   {
      for(var = grp->var; var; var = var->next)
      {
         if (var->varid == varid)
         {
            attlist = var->att;
            break;
         }
      }
      if (!var)
         return NC_ENOTVAR;
   }

   /* Now find the attribute by name or number. If a name is provided,
    * ignore the attnum. */
   for (*att = attlist; *att; *att = (*att)->next)
      if ((name && !strcmp((*att)->name, name)) ||
          (!name && (*att)->attnum == attnum))
         return NC_NOERR;

   /* If we get here, we couldn't find the attribute. */
   return NC_ENOTATT;
}

/* Given an ncid, varid, and name or attnum, find and return pointer
   to NC_ATT_INFO_T metadata. */
int
nc4_find_nc_att(int ncid, int varid, const char *name, int attnum,
            NC_ATT_INFO_T **att)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T *attlist = NULL;
   int retval;

   LOG((4, "nc4_find_nc_att: ncid 0x%x varid %d name %s attnum %d",
        ncid, varid, name, attnum));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;
   assert(grp && h5);

   /* Get either the global or a variable attribute list. */
   if (varid == NC_GLOBAL)
      attlist = grp->att;
   else
   {
      for(var = grp->var; var; var = var->next)
      {
         if (var->varid == varid)
         {
            attlist = var->att;
            break;
         }
      }
      if (!var)
         return NC_ENOTVAR;
   }

   /* Now find the attribute by name or number. If a name is provided, ignore the attnum. */
   for (*att = attlist; *att; *att = (*att)->next)
      if ((name && !strcmp((*att)->name, name)) ||
          (!name && (*att)->attnum == attnum))
         return NC_NOERR;

   /* If we get here, we couldn't find the attribute. */
   return NC_ENOTATT;
}

void
nc4_file_list_free(void)
{
    free_NCList();
}


int
NC4_new_nc(NC** ncpp)
{
    NC_FILE_INFO_T** ncp;
    /* Allocate memory for this info. */
    if (!(ncp = calloc(1, sizeof(NC_FILE_INFO_T))))
       return NC_ENOMEM;
    if(ncpp) *ncpp = (NC*)ncp;
    return NC_NOERR;
}

int
nc4_file_list_add(NC_FILE_INFO_T** ncp, NC_Dispatch* dispatch)
{
    NC_FILE_INFO_T *nc;
    int status = NC_NOERR;

    /* Allocate memory for this info; use the dispatcher to do this */
    status = dispatch->new_nc((NC**)&nc);
    if(status) return status;

    /* Add this file to the list. */
    if ((status = add_to_NCList((NC *)nc)))
    {
       if(nc && nc->ext_ncid > 0)
       {
          del_from_NCList((NC *)nc);
          free(nc);
       }
       return status;
    }

    /* Return a pointer to the new struct. */
    if(ncp)
       *ncp = nc;

    return NC_NOERR;
}

/* Remove a NC_FILE_INFO_T from the linked list. This will nc_free the
   memory too. */
void
nc4_file_list_del(NC_FILE_INFO_T *nc)
{
   /* Remove file from master list. */
   del_from_NCList((NC *)nc);
   free(nc);
}


/* Given an id, walk the list and find the appropriate
   NC_FILE_INFO_T. */
NC_FILE_INFO_T*
nc4_find_nc_file(int ext_ncid)
{
   return (NC_FILE_INFO_T*)find_in_NCList(ext_ncid);
}


/* Add to the end of a var list. Return a pointer to the newly
 * added var. */
int
nc4_var_list_add(NC_VAR_INFO_T **list, NC_VAR_INFO_T **var)
{
   NC_VAR_INFO_T *v;

   /* Allocate storage for new variable. */
   if (!(*var = calloc(1, sizeof(NC_VAR_INFO_T))))
      return NC_ENOMEM;

   /* Go to the end of the list and set the last one to point at our
    * new var, or, if the list is empty, our new var becomes the
    * list. */
   if(*list)
   {
      for (v = *list; v; v = v->next)
         if (!v->next)
            break;
      v->next = *var;
      (*var)->prev = v;
   }
   else
      *list = *var;

   /* These are the HDF5-1.8.4 defaults. */
   (*var)->chunk_cache_size = nc4_chunk_cache_size;
   (*var)->chunk_cache_nelems = nc4_chunk_cache_nelems;
   (*var)->chunk_cache_preemption = nc4_chunk_cache_preemption;

   return NC_NOERR;
}

/* Add to the beginning of a dim list. */
int
nc4_dim_list_add(NC_DIM_INFO_T **list)
{
   NC_DIM_INFO_T *dim;
   if (!(dim = calloc(1, sizeof(NC_DIM_INFO_T))))
      return NC_ENOMEM;
   if(*list)
      (*list)->prev = dim;
   dim->next = *list;
   *list = dim;
   return NC_NOERR;
}

/* Add to the beginning of a dim list. */
int
nc4_dim_list_add2(NC_DIM_INFO_T **list, NC_DIM_INFO_T **new_dim)
{
   NC_DIM_INFO_T *dim;
   if (!(dim = calloc(1, sizeof(NC_DIM_INFO_T))))
      return NC_ENOMEM;
   if(*list)
      (*list)->prev = dim;
   dim->next = *list;
   *list = dim;

   /* Return pointer to new dimension. */
   if (new_dim)
      *new_dim = dim;
   return NC_NOERR;
}

/* Add to the end of an att list. */
int
nc4_att_list_add(NC_ATT_INFO_T **list)
{
   NC_ATT_INFO_T *att, *a1;
   if (!(att = calloc(1, sizeof(NC_ATT_INFO_T))))
      return NC_ENOMEM;
   if (*list)
   {
      for (a1 = *list; a1; a1 = a1->next)
         if (!a1->next)
            break;
      a1->next = att;
      att->prev = a1;
   }
   else
   {
      *list = att;
   }

   return NC_NOERR;
}

/* Add to the end of a group list. Can't use 0 as a new_nc_grpid -
 * it's reserverd for the root group. */
int
nc4_grp_list_add(NC_GRP_INFO_T **list, int new_nc_grpid,
                 NC_GRP_INFO_T *parent_grp, NC_FILE_INFO_T *nc,
                 char *name, NC_GRP_INFO_T **grp)
{
   NC_GRP_INFO_T *g;

   LOG((3, "grp_list_add: new_nc_grpid %d name %s ",
        new_nc_grpid, name));

   /* Get the memory to store this groups info. */
   if (!(*grp = calloc(1, sizeof(NC_GRP_INFO_T))))
      return NC_ENOMEM;

   /* If the list is not NULL, add this group to it. Otherwise, this
    * group structure becomes the list. */
   if (*list)
   {
      /* Move to end of the list. */
      for (g = *list; g; g = g->next)
         if (!g->next)
            break;
      g->next = *grp; /* Add grp to end of list. */
      (*grp)->prev = g;
   }
   else
   {
      *list = *grp;
   }

   /* Fill in this group's information. */
   (*grp)->nc_grpid = new_nc_grpid;
   (*grp)->parent = parent_grp;
   if (!((*grp)->name = malloc((strlen(name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy((*grp)->name, name);
   (*grp)->file = nc;

   return NC_NOERR;
}

/* Names for groups, variables, and types must not be the same. This
 * function checks that a proposed name is not already in
 * use. Normalzation of UTF8 strings should happen before this
 * function is called. */
int
nc4_check_dup_name(NC_GRP_INFO_T *grp, char *name)
{
   NC_TYPE_INFO_T *type;
   NC_GRP_INFO_T *g;
   NC_VAR_INFO_T *var;

   /* Any types of this name? */
   for (type = grp->type; type; type = type->next)
      if (!strcmp(type->name, name))
         return NC_ENAMEINUSE;

   /* Any child groups of this name? */
   for (g = grp->children; g; g = g->next)
      if (!strcmp(g->name, name))
         return NC_ENAMEINUSE;

   /* Any variables of this name? */
   for (var = grp->var; var; var = var->next)
      if (!strcmp(var->name, name))
         return NC_ENAMEINUSE;

   return NC_NOERR;
}

/* Add to the end of a type list. */
int
nc4_type_list_add(NC_TYPE_INFO_T **list, NC_TYPE_INFO_T **new_type)
{
   NC_TYPE_INFO_T *type, *t;

   if (!(type = calloc(1, sizeof(NC_TYPE_INFO_T))))
      return NC_ENOMEM;

   if (*list)
   {
      for (t = *list; t; t = t->next)
         if (!t->next)
            break;
      t->next = type;
      type->prev = t;
   }
   else
   {
      *list = type;
   }

   if (new_type)
      *new_type = type;

   return NC_NOERR;
}

/* Add to the end of a compound field list. */
int
nc4_field_list_add(NC_FIELD_INFO_T **list, int fieldid, const char *name,
                   size_t offset, hid_t field_hdf_typeid, hid_t native_typeid,
                   nc_type xtype, int ndims, const int *dim_sizesp)
{
   NC_FIELD_INFO_T *field, *f;
   int i;

   /* Name has already been checked and UTF8 normalized. */
   if (!name)
      return NC_EINVAL;

   /* Allocate storage for this field information. */
   if (!(field = calloc(1, sizeof(NC_FIELD_INFO_T))))
      return NC_ENOMEM;

   /* Add this field to list. */
   if (*list)
   {
      for (f = *list; f; f = f->next)
         if (!f->next)
            break;
      f->next = field;
      field->prev = f;
   }
   else
   {
      *list = field;
   }

   /* Store the information about this field. */
   field->fieldid = fieldid;
   if (!(field->name = malloc((strlen(name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(field->name, name);
   field->hdf_typeid = field_hdf_typeid;
   field->native_typeid = native_typeid;
   field->nctype = xtype;
   field->offset = offset;
   field->ndims = ndims;
   if (ndims)
   {
      if (!(field->dim_size = malloc(ndims * sizeof(int))))
         return NC_ENOMEM;
      for (i = 0; i < ndims; i++)
         field->dim_size[i] = dim_sizesp[i];
   }

   return NC_NOERR;
}

/* Add a member to an enum type. */
int
nc4_enum_member_add(NC_ENUM_MEMBER_INFO_T **list, size_t size,
                    const char *name, const void *value)
{
   NC_ENUM_MEMBER_INFO_T *member, *m;

   /* Name has already been checked. */
   assert(name && size > 0 && value);
   LOG((4, "nc4_enum_member_add: size %d name %s", size, name));

   /* Allocate storage for this field information. */
   if (!(member = calloc(1, sizeof(NC_ENUM_MEMBER_INFO_T))) ||
       !(member->value = calloc(1, size)))
      return NC_ENOMEM;

   /* Add this field to list. */
   if (*list)
   {
      for (m = *list; m; m = m->next)
         if (!m->next)
            break;
      m->next = member;
      member->prev = m;
   }
   else
   {
      *list = member;
   }

   /* Store the information about this member. */
   if (!(member->name = malloc((strlen(name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(member->name, name);
   memcpy(member->value, value, size);

   return NC_NOERR;
}

/* Delete a var from a var list, and free the memory. */
static int
var_list_del(NC_VAR_INFO_T **list, NC_VAR_INFO_T *var)
{
   NC_ATT_INFO_T *a, *att;
   int ret;

   /* First delete all the attributes attached to this var. */
   att = (*list)->att;
   while (att)
   {
      a = att->next;
      if ((ret = nc4_att_list_del(&var->att, att)))
         return ret;
      att = a;
   }

   /* Free some things that may be allocated. */
   if (var->chunksizes)
      free(var->chunksizes);
   if (var->hdf5_name)
      free(var->hdf5_name);
   if (var->name)
      free(var->name);
   if (var->dimids)
      free(var->dimids);
   if (var->dim)
      free(var->dim);

   /* Remove the var from the linked list. */
   if(*list == var)
      *list = var->next;
   else
      var->prev->next = var->next;

   if(var->next)
      var->next->prev = var->prev;

   /* Delete any fill value allocation. This must be done before the
    * type_info is freed. */
   if (var->fill_value)
   {
      if (var->hdf_datasetid)
      {
         if (var->type_info->class == NC_VLEN)
            nc_free_vlen((nc_vlen_t *)var->fill_value);
         else if (var->type_info->nc_typeid == NC_STRING)
            free(*(char **)var->fill_value);
      }
      free(var->fill_value);
   }

   /* For atomic types we have allocated space for type information. */
/*   if (var->hdf_datasetid && var->xtype <= NC_STRING)*/
   if (var->xtype <= NC_STRING)
   {
      if (var->type_info->native_typeid)
         if ((H5Tclose(var->type_info->native_typeid)) < 0)
            return NC_EHDFERR;

      /* Only need to close the hdf_typeid when it was obtained with
       * H5Dget_type (which happens when reading a file, but not when
       * creating a variable). */
      if (var->type_info->close_hdf_typeid || var->xtype == NC_STRING)
         if ((H5Tclose(var->type_info->hdf_typeid)) < 0)
            return NC_EHDFERR;

      /* Free the name. */
      if (var->type_info->name)
         free(var->type_info->name);

      free(var->type_info);
   }

   /* Delete any HDF5 dimscale objid information. */
   if (var->dimscale_hdf5_objids)
      free(var->dimscale_hdf5_objids);

   /* Delete information about the attachment status of dimscales. */
   if (var->dimscale_attached)
      free(var->dimscale_attached);

   /* Delete the var. */
   free(var);

   return NC_NOERR;
}

/* Delete a field from a field list, and nc_free the memory. */
static void
field_list_del(NC_FIELD_INFO_T **list, NC_FIELD_INFO_T *field)
{

   /* Take this field out of the list. */
   if(*list == field)
      *list = field->next;
   else
      field->prev->next = field->next;

   if(field->next)
      field->next->prev = field->prev;

   /* Free some stuff. */
   if (field->name)
      free(field->name);
   if (field->dim_size)
      free(field->dim_size);

   /* Nc_Free the memory. */
   free(field);
}

/* Delete a type from a type list, and nc_free the memory. */
int
type_list_del(NC_TYPE_INFO_T **list, NC_TYPE_INFO_T *type)
{
   NC_FIELD_INFO_T *field, *f;
   NC_ENUM_MEMBER_INFO_T *enum_member, *em;

   /* Close any open user-defined HDF5 typieds. */
   if (type->hdf_typeid)
   {
      if (H5Tclose(type->hdf_typeid) < 0)
         return NC_EHDFERR;
   }
   if (type->native_typeid)
   {
      if (H5Tclose(type->native_typeid) < 0)
         return NC_EHDFERR;
   }

   /* Free the name. */
   if (type->name)
      free(type->name);

   /* Delete all the fields in this type (there will be some if its a
    * compound). */
   field = type->field;
   while (field)
   {
      f = field->next;
      field_list_del(&type->field, field);
      field = f;
   }

   /* Delete all the enum_members, if any. */
   enum_member = type->enum_member;
   while (enum_member)
   {
      em = enum_member->next;
      free(enum_member->value);
      free(enum_member->name);
      free(enum_member);
      enum_member = em;
   }

   /* Take this type out of the list. */
   if(*list == type)
      *list = type->next;
   else
      type->prev->next = type->next;

   if(type->next)
      type->next->prev = type->prev;

   /* Nc_Free the memory. */
   free(type);

   return NC_NOERR;
}

/* Delete a del from a var list, and nc_free the memory. */
int
nc4_dim_list_del(NC_DIM_INFO_T **list, NC_DIM_INFO_T *dim)
{
   /* Take this dimension out of the list. */
   if(*list == dim)
      *list = dim->next;
   else
      dim->prev->next = dim->next;

   if(dim->next)
      dim->next->prev = dim->prev;

   /* Free memory allocated for names. */
   if (dim->name)
      free(dim->name);
   if (dim->old_name)
      free(dim->old_name);

   free(dim);
   return NC_NOERR;
}

/* Remove a NC_GRP_INFO_T from the linked list. This will nc_free the
   memory too. */
static void
grp_list_del(NC_GRP_INFO_T **list, NC_GRP_INFO_T *grp)
{
   if(*list == grp)
      *list = grp->next;
   else
      grp->prev->next = grp->next;

   if(grp->next)
      grp->next->prev = grp->prev;

   free(grp);
}

/* Recursively delete the data for a group (and everything it
 * contains) in our internal metadata store. */
int
nc4_rec_grp_del(NC_GRP_INFO_T **list, NC_GRP_INFO_T *grp)
{
   NC_GRP_INFO_T *g, *c;
   NC_VAR_INFO_T *v, *var;
   NC_ATT_INFO_T *a, *att;
   NC_DIM_INFO_T *d, *dim;
   NC_TYPE_INFO_T *type, *t;
   int retval;

   assert(grp);
   LOG((3, "nc4_rec_grp_del: grp->name %s", grp->name));

   /* Recursively call this function for each child, if any, stopping
    * if there is an error. */
   g = grp->children;
   while(g)
   {
      c = g->next;
      if ((retval = nc4_rec_grp_del(&(grp->children), g)))
         return retval;
      g = c;
   }

   /* Delete all the list contents for vars, dims, and atts, in each
    * group. */
   att = grp->att;
   while (att)
   {
      LOG((4, "nc4_rec_grp_del: deleting att %s", att->name));
      a = att->next;
      if ((retval = nc4_att_list_del(&grp->att, att)))
         return retval;
      att = a;
   }

   /* Delete all vars. */
   var = grp->var;
   while (var)
   {
      LOG((4, "nc4_rec_grp_del: deleting var %s", var->name));
      /* Close HDF5 dataset associated with this var, unless it's a
       * scale. */
      if (var->hdf_datasetid && !var->dimscale &&
          H5Dclose(var->hdf_datasetid) < 0)
         return NC_EHDFERR;
      v = var->next;
      if ((retval = var_list_del(&grp->var, var)))
         return retval;
      var = v;
   }

   /* Delete all dims. */
   dim = grp->dim;
   while (dim)
   {
      LOG((4, "nc4_rec_grp_del: deleting dim %s", dim->name));
      /* Close HDF5 dataset associated with this dim. */
      if (dim->hdf_dimscaleid && H5Dclose(dim->hdf_dimscaleid) < 0)
         return NC_EHDFERR;
      d = dim->next;
      if ((retval = nc4_dim_list_del(&grp->dim, dim)))
         return retval;
      dim = d;
   }

   /* Delete all types. */
   type = grp->type;
   while (type)
   {
      LOG((4, "nc4_rec_grp_del: deleting type %s", type->name));
      t = type->next;
      if ((retval = type_list_del(&grp->type, type)))
         return retval;
      type = t;
   }

   /* Tell HDF5 we're closing this group. */
   LOG((4, "nc4_rec_grp_del: closing group %s", grp->name));
   if (grp->hdf_grpid && H5Gclose(grp->hdf_grpid) < 0)
      return NC_EHDFERR;

   /* Free the name. */
   free(grp->name);

   /* Finally, redirect pointers around this entry in the list, and
    * nc_free its memory. */
   grp_list_del(list, grp);

   return NC_NOERR;
}

/* Remove a NC_ATT_INFO_T from the linked list. This will nc_free the
   memory too.
*/
int
nc4_att_list_del(NC_ATT_INFO_T **list, NC_ATT_INFO_T *att)
{
   int i;

   /* Take this att out of the list. */
   if(*list == att)
      *list = att->next;
   else
      att->prev->next = att->next;

   if(att->next)
      att->next->prev = att->prev;

   /* Free memory that was malloced to hold data for this
    * attribute. */
   if (att->data)
      free(att->data);

   /* Free the name. */
   if (att->name)
      free(att->name);

   /* Close the HDF5 typeid. */
   if (att->native_typeid && H5Tclose(att->native_typeid) < 0)
      return NC_EHDFERR;

   /* If this is a string array attribute, delete all members of the
    * string array, then delete the array of pointers to strings. (The
    * array was filled with pointers by HDF5 when the att was read,
    * and memory for each string was allocated by HDF5. That's why I
    * use free and not nc_free, because the netCDF library didn't
    * allocate the memory that is being freed.) */
   if (att->stdata)
   {
      for (i = 0; i < att->len; i++)
         free(att->stdata[i]);
      free(att->stdata);
   }

   /* If this att has vlen data, release it. */
   if (att->vldata)
   {
      for (i = 0; i < att->len; i++)
         nc_free_vlen(&att->vldata[i]);
      free(att->vldata);
   }

   free(att);
   return NC_NOERR;
}

/* Normalize a UTF8 name. Put the result in norm_name, which can be
 * NC_MAX_NAME + 1 in size. This function makes sure the free() gets
 * called on the return from utf8proc_NFC, and also ensures that the
 * name is not too long. */
int
nc4_normalize_name(const char *name, char *norm_name)
{
   char *temp_name;
   if (!(temp_name = (char *)utf8proc_NFC((const unsigned char *)name)))
      return NC_EINVAL;
   if (strlen(temp_name) > NC_MAX_NAME)
   {
      free(temp_name);
      return NC_EMAXNAME;
   }
   strcpy(norm_name, temp_name);
   free(temp_name);
   return NC_NOERR;
}

/* Print out a bunch of info to stderr about the metadata for
   debugging purposes. */
#ifdef LOGGING
/* Use this to set the global log level. Set it to NC_TURN_OFF_LOGGING
   (-1) to turn off all logging. Set it to 0 to show only errors, and
   to higher numbers to show more and more logging details. */
int
nc_set_log_level(int new_level)
{
   /* If the user wants to completely turn off logging, turn off HDF5
      logging too. Now I truely can't think of what to do if this
      fails, so just ignore the return code. */
   if (new_level == NC_TURN_OFF_LOGGING)
   {
      H5Eset_auto(NULL, NULL);
      LOG((1, "HDF5 error messages turned off!"));
   }

   /* Do we need to turn HDF5 logging back on? */
   if (new_level > NC_TURN_OFF_LOGGING &&
       nc_log_level <= NC_TURN_OFF_LOGGING)
   {
      if (H5Eset_auto((H5E_auto_t)&H5Eprint, stderr) < 0)
         LOG((0, "H5Eset_auto failed!"));
      LOG((1, "HDF5 error messages turned on."));
   }

   /* Now remember the new level. */
   nc_log_level = new_level;
   LOG((4, "log_level changed to %d", nc_log_level));
   return 0;
}

/* Recursively print the metadata of a group. */
#define MAX_NESTS 10
static int
rec_print_metadata(NC_GRP_INFO_T *grp, int *tab_count)
{
   NC_GRP_INFO_T *g;
   NC_ATT_INFO_T *att;
   NC_VAR_INFO_T *var;
   NC_DIM_INFO_T *dim;
   NC_TYPE_INFO_T *type;
   NC_FIELD_INFO_T *field;
   char tabs[MAX_NESTS] = "";
   char dims_string[NC_MAX_DIMS*4];
   char temp_string[10];
   int t, retval, d;

   /* Come up with a number of tabs relative to the group. */
   for (t = 0; t < *tab_count && t < MAX_NESTS; t++)
      strcat(tabs, "\t");

   LOG((2, "%s GROUP - %s nc_grpid: %d nvars: %d natts: %d",
        tabs, grp->name, grp->nc_grpid, grp->nvars, grp->natts));

   for(att = grp->att; att; att = att->next)
      LOG((2, "%s GROUP ATTRIBUTE - attnum: %d name: %s type: %d len: %d",
           tabs, att->attnum, att->name, att->xtype, att->len));

   /* To display dims starting with 0 and going up, go through list is
    * reverse order. */
   for(dim = grp->dim; dim && dim->next; dim = dim->next)
      ;
   for( ; dim; dim = dim->prev)
      LOG((2, "%s DIMENSION - dimid: %d name: %s len: %d unlimited: %d",
           tabs, dim->dimid, dim->name, dim->len, dim->unlimited));

   /* To display vars starting with 0 and going up, go through list is
    * reverse order. */
   for(var = grp->var; var && var->next; var = var->next)
      ;
   for( ; var; var = var->prev)
   {
      strcpy(dims_string, "");
      for (d = 0; d < var->ndims; d++)
      {
         sprintf(temp_string, " %d", var->dimids[d]);
         strcat(dims_string, temp_string);
      }
      LOG((2, "%s VARIABLE - varid: %d name: %s type: %d ndims: %d dimscale: %d dimids:%s",
           tabs, var->varid, var->name, var->xtype, var->ndims, var->dimscale,
           dims_string));
      for(att = var->att; att; att = att->next)
         LOG((2, "%s VAR ATTRIBUTE - attnum: %d name: %s type: %d len: %d",
              tabs, att->attnum, att->name, att->xtype, att->len));
   }

   for (type = grp->type; type; type = type->next)
   {
      LOG((2, "%s TYPE - nc_typeid: %d hdf_typeid: 0x%x size: %d committed: %d "
           "name: %s num_fields: %d base_nc_type: %d", tabs, type->nc_typeid,
           type->hdf_typeid, type->size, type->committed, type->name,
           type->num_fields, type->base_nc_type));
      /* Is this a compound type? */
      if (type->class == NC_COMPOUND)
      {
         LOG((3, "compound type"));
         for (field = type->field; field; field = field->next)
            LOG((4, "field %s offset %d nctype %d ndims %d", field->name,
                 field->offset, field->nctype, field->ndims));
      }
      else if (type->class == NC_VLEN)
         LOG((3, "VLEN type"));
      else if (type->class == NC_OPAQUE)
         LOG((3, "Opaque type"));
      else if (type->class == NC_ENUM)
         LOG((3, "Enum type"));
      else
      {
         LOG((0, "Unknown class: %d", type->class));
         return NC_EBADTYPE;
      }
   }

   /* Call self for each child of this group. */
   if (grp->children)
   {
      (*tab_count)++;
      for (g = grp->children; g; g = g->next)
         if ((retval = rec_print_metadata(g, tab_count)))
            return retval;
      (*tab_count)--;
   }

   return NC_NOERR;
}

/* Print out the internal metadata for a file. This is useful to check
 * that netCDF is working! Nonetheless, this function will print
 * nothing if logging is not set to at least two. */
int
log_metadata_nc(NC_FILE_INFO_T *nc)
{
   NC_HDF5_FILE_INFO_T *h5 = nc->nc4_info;
   int tab_count = 0;

   LOG((2, "*** NetCDF-4 Internal Metadata: int_ncid 0x%x ext_ncid 0x%x",
        nc->int_ncid, nc->ext_ncid));
   if (!h5)
   {
      LOG((2, "This is a netCDF-3 file."));
      return NC_NOERR;
   }
   LOG((2, "FILE - hdfid: 0x%x path: %s cmode: 0x%x parallel: %d redef: %d "
        "fill_mode: %d no_write: %d next_nc_grpid: %d",	h5->hdfid, h5->path,
        h5->cmode, h5->parallel, h5->redef, h5->fill_mode, h5->no_write,
        h5->next_nc_grpid));
   return rec_print_metadata(h5->root_grp, &tab_count);
}

#endif /*LOGGING */

/* Show the in-memory metadata for a netcdf file. */
int
NC4_show_metadata(int ncid)
{
   int retval = NC_NOERR;
#ifdef LOGGING
   NC_FILE_INFO_T *nc;
   int old_log_level = nc_log_level;

   /* Find file metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Log level must be 2 to see metadata. */
   nc_log_level = 2;
   retval = log_metadata_nc(nc);
   nc_log_level = old_log_level;
#endif /*LOGGING*/
   return retval;
}
