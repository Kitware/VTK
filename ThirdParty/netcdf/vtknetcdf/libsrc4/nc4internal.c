/* Copyright 2003-2018, University Corporation for Atmospheric
 * Research. See the COPYRIGHT file for copying and redistribution
 * conditions.
 */
/**
 * @file
 * @internal
 * Internal netcdf-4 functions.
 *
 * This file contains functions internal to the netcdf4 library. None of
 * the functions in this file are exposed in the exetnal API. These
 * functions all relate to the manipulation of netcdf-4's in-memory
 * buffer of metadata information, i.e. the linked list of NC
 * structs.
 *
 * @author Ed Hartnett
 */
#include "config.h"
#include "nc4internal.h"
#include "nc.h" /* from libsrc */
#include "ncdispatch.h" /* from libdispatch */
#include "ncutf8.h"
#include "H5DSpublic.h"

#undef DEBUGH5

#ifdef DEBUGH5
/**
 * @internal Provide a catchable error reporting function
 *
 * @param ignored Ignored.
 *
 * @return 0 for success.
 */
static herr_t
h5catch(void* ignored)
{
   H5Eprint(NULL);
   return 0;
}
#endif

/* These are the default chunk cache sizes for HDF5 files created or
 * opened with netCDF-4. */
extern size_t nc4_chunk_cache_size;
extern size_t nc4_chunk_cache_nelems;
extern float nc4_chunk_cache_preemption;

#ifdef LOGGING
/* This is the severity level of messages which will be logged. Use
   severity 0 for errors, 1 for important log messages, 2 for less
   important, etc. */
int nc_log_level = NC_TURN_OFF_LOGGING;

#endif /* LOGGING */

int nc4_hdf5_initialized = 0; /**< True if initialization has happened. */

/**
 * @internal Provide a wrapper for H5Eset_auto
 * @param func Pointer to func.
 * @param client_data Client data.
 *
 * @return 0 for success
 */
static herr_t
set_auto(void* func, void *client_data)
{
#ifdef DEBUGH5
   return H5Eset_auto2(H5E_DEFAULT,(H5E_auto2_t)h5catch,client_data);
#else
   return H5Eset_auto2(H5E_DEFAULT,(H5E_auto2_t)func,client_data);
#endif
}

/**
 * @internal Provide a function to do any necessary initialization of
 * the HDF5 library.
 */
void
nc4_hdf5_initialize(void)
{
   if (set_auto(NULL, NULL) < 0)
      LOG((0, "Couldn't turn off HDF5 error messages!"));
   LOG((1, "HDF5 error messages have been turned off."));
   nc4_hdf5_initialized = 1;
}

/**
 * @internal Check and normalize and name.
 *
 * @param name Name to normalize.
 * @param norm_name The normalized name.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EMAXNAME Name too long.
 * @return ::NC_EINVAL NULL given for name.
 * @author Dennis Heimbigner
 */
int
nc4_check_name(const char *name, char *norm_name)
{
   char *temp;
   int retval;

   /* Check for NULL. */
   if (!name)
      return NC_EINVAL;

   assert(norm_name);

   /* Check for NULL. */
   if (!name)
      return NC_EINVAL;

   /* Check the length. */
   if (strlen(name) > NC_MAX_NAME)
      return NC_EMAXNAME;

   /* Make sure this is a valid netcdf name. This should be done
    * before the name is normalized, because it gives better error
    * codes for bad utf8 strings. */
   if ((retval = NC_check_name(name)))
      return retval;

   /* Normalize the name. */
   retval = nc_utf8_normalize((const unsigned char *)name,(unsigned char**)&temp);
   if(retval != NC_NOERR)
      return retval;

   if(strlen(temp) > NC_MAX_NAME) {
      free(temp);
      return NC_EMAXNAME;
   }

   strcpy(norm_name, temp);
   free(temp);

   return NC_NOERR;
}

/**
 * @internal Given a varid, return the maximum length of a dimension
 * using dimid.
 *
 * @param grp Pointer to group info struct.
 * @param varid Variable ID.
 * @param dimid Dimension ID.
 * @param maxlen Pointer that gets the max length.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
find_var_dim_max_length(NC_GRP_INFO_T *grp, int varid, int dimid, size_t *maxlen)
{
   hid_t datasetid = 0, spaceid = 0;
   NC_VAR_INFO_T *var;
   hsize_t *h5dimlen = NULL, *h5dimlenmax = NULL;
   int d, dataset_ndims = 0;
   int retval = NC_NOERR;

   *maxlen = 0;

   /* Find this var. */
   if (varid < 0 || varid >= grp->vars.nelems)
      return NC_ENOTVAR;
   var = grp->vars.value[varid];
   if (!var) return NC_ENOTVAR;
   assert(var->varid == varid);

   /* If the var hasn't been created yet, its size is 0. */
   if (!var->created)
   {
      *maxlen = 0;
   }
   else
   {
      /* Get the number of records in the dataset. */
      if ((retval = nc4_open_var_grp2(grp, var->varid, &datasetid)))
         BAIL(retval);
      if ((spaceid = H5Dget_space(datasetid)) < 0)
         BAIL(NC_EHDFERR);

      /* If it's a scalar dataset, it has length one. */
      if (H5Sget_simple_extent_type(spaceid) == H5S_SCALAR)
      {
         *maxlen = (var->dimids && var->dimids[0] == dimid) ? 1 : 0;
      }
      else
      {
         /* Check to make sure ndims is right, then get the len of each
            dim in the space. */
         if ((dataset_ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
            BAIL(NC_EHDFERR);
         if (dataset_ndims != var->ndims)
            BAIL(NC_EHDFERR);
         if (!(h5dimlen = malloc(dataset_ndims * sizeof(hsize_t))))
            BAIL(NC_ENOMEM);
         if (!(h5dimlenmax = malloc(dataset_ndims * sizeof(hsize_t))))
            BAIL(NC_ENOMEM);
         if ((dataset_ndims = H5Sget_simple_extent_dims(spaceid,
                                                        h5dimlen, h5dimlenmax)) < 0)
            BAIL(NC_EHDFERR);
         LOG((5, "find_var_dim_max_length: varid %d len %d max: %d",
              varid, (int)h5dimlen[0], (int)h5dimlenmax[0]));
         for (d=0; d<dataset_ndims; d++) {
            if (var->dimids[d] == dimid) {
               *maxlen = *maxlen > h5dimlen[d] ? *maxlen : h5dimlen[d];
            }
         }
      }
   }

exit:
   if (spaceid > 0 && H5Sclose(spaceid) < 0)
      BAIL2(NC_EHDFERR);
   if (h5dimlen) free(h5dimlen);
   if (h5dimlenmax) free(h5dimlenmax);
   return retval;
}

/**
 * @internal Given an NC pointer, add the necessary stuff for a
 * netcdf-4 file.
 *
 * @param nc Pointer to file's NC struct.
 * @param path The file name of the new file.
 * @param mode The mode flag.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_nc4f_list_add(NC *nc, const char *path, int mode)
{
   NC_HDF5_FILE_INFO_T *h5;

   assert(nc && !NC4_DATA(nc) && path);

   /* We need to malloc and
      initialize the substructure NC_HDF_FILE_INFO_T. */
   if (!(h5 = calloc(1, sizeof(NC_HDF5_FILE_INFO_T))))
      return NC_ENOMEM;
   NC4_DATA_SET(nc,h5);
   h5->controller = nc;

   /* Hang on to cmode, and note that we're in define mode. */
   h5->cmode = mode | NC_INDEF;

   /* The next_typeid needs to be set beyond the end of our atomic
    * types. */
   h5->next_typeid = NC_FIRSTUSERTYPEID;

   /* There's always at least one open group - the root
    * group. Allocate space for one group's worth of information. Set
    * its hdf id, name, and a pointer to it's file structure. */
   return nc4_grp_list_add(&(h5->root_grp), h5->next_nc_grpid++,
                           NULL, nc, NC_GROUP_NAME, NULL);
}

/**
 * @internal Given an ncid, find the relevant group and return a
 * pointer to it, return an error of this is not a netcdf-4 file (or
 * if strict nc3 is turned on for this file.)
 *
 * @param ncid File and group ID.
 * @param grp Pointer that gets pointer to group info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOTNC4 Not a netCDF-4 file.
 * @return ::NC_ESTRICTNC3 Not allowed for classic model.
 * @author Ed Hartnett
 */
int
nc4_find_nc4_grp(int ncid, NC_GRP_INFO_T **grp)
{
   NC_HDF5_FILE_INFO_T* h5;
   NC *f = nc4_find_nc_file(ncid,&h5);
   if(f == NULL) return NC_EBADID;

   /* No netcdf-3 files allowed! */
   if (!h5) return NC_ENOTNC4;
   assert(h5->root_grp);

   /* This function demands netcdf-4 files without strict nc3
    * rules.*/
   if (h5->cmode & NC_CLASSIC_MODEL) return NC_ESTRICTNC3;

   /* If we can't find it, the grp id part of ncid is bad. */
   if (!(*grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
      return NC_EBADID;
   return NC_NOERR;
}

/**
 * @internal Given an ncid, find the relevant group and return a
 * pointer to it, also set a pointer to the nc4_info struct of the
 * related file. For netcdf-3 files, *h5 will be set to NULL.
 *
 * @param ncid File and group ID.
 * @param grpp Pointer that gets pointer to group info struct.
 * @param h5p Pointer to HDF5 file struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
nc4_find_grp_h5(int ncid, NC_GRP_INFO_T **grpp, NC_HDF5_FILE_INFO_T **h5p)
{
   NC_HDF5_FILE_INFO_T *h5;
   NC_GRP_INFO_T *grp;
   NC *f = nc4_find_nc_file(ncid,&h5);
   if(f == NULL) return NC_EBADID;
   if (h5) {
      assert(h5->root_grp);
      /* If we can't find it, the grp id part of ncid is bad. */
      if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
         return NC_EBADID;
      h5 = (grp)->nc4_info;
      assert(h5);
   } else {
      h5 = NULL;
      grp = NULL;
   }
   if(h5p) *h5p = h5;
   if(grpp) *grpp = grp;
   return NC_NOERR;
}

/**
 * @internal Find info for this file and group, and set pointer to each.
 *
 * @param ncid File and group ID.
 * @param nc Pointer that gets a pointer to the file's NC struct.
 * @param grpp Pointer that gets a pointer to the group struct.
 * @param h5p Pointer that gets HDF5 file struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
nc4_find_nc_grp_h5(int ncid, NC **nc, NC_GRP_INFO_T **grpp,
                   NC_HDF5_FILE_INFO_T **h5p)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T* h5;
   NC *f = nc4_find_nc_file(ncid,&h5);

   if(f == NULL) return NC_EBADID;
   *nc = f;

   if (h5) {
      assert(h5->root_grp);
      /* If we can't find it, the grp id part of ncid is bad. */
      if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
         return NC_EBADID;

      h5 = (grp)->nc4_info;
      assert(h5);
   } else {
      h5 = NULL;
      grp = NULL;
   }
   if(h5p) *h5p = h5;
   if(grpp) *grpp = grp;
   return NC_NOERR;
}

/**
 * @internal Recursively hunt for a group id.
 *
 * @param start_grp Pointer to group where search should be started.
 * @param target_nc_grpid Group ID to be found.
 *
 * @return Pointer to group info struct, or NULL if not found.
 * @author Ed Hartnett
 */
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
      for (g = start_grp->children; g; g = g->l.next)
         if ((res = nc4_rec_find_grp(g, target_nc_grpid)))
            return res;

   /* Can't find it. Fate, why do you mock me? */
   return NULL;
}

/**
 * @internal Given an ncid and varid, get pointers to the group and var
 * metadata.
 *
 * @param nc Pointer to file's NC struct.
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param grp Pointer that gets pointer to group info.
 * @param var Pointer that gets pointer to var info.
 *
 * @return ::NC_NOERR No error.
 */
int
nc4_find_g_var_nc(NC *nc, int ncid, int varid,
                  NC_GRP_INFO_T **grp, NC_VAR_INFO_T **var)
{
   NC_HDF5_FILE_INFO_T* h5 = NC4_DATA(nc);

   /* Find the group info. */
   assert(grp && var && h5 && h5->root_grp);
   *grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK));

   /* It is possible for *grp to be NULL. If it is,
      return an error. */
   if(*grp == NULL)
      return NC_EBADID;

   /* Find the var info. */
   if (varid < 0 || varid >= (*grp)->vars.nelems)
      return NC_ENOTVAR;
   (*var) = (*grp)->vars.value[varid];

   return NC_NOERR;
}

/**
 * @internal Find a dim in a grp (or its parents).
 *
 * @param grp Pointer to group info struct.
 * @param dimid Dimension ID to find.
 * @param dim Pointer that gets pointer to dim info if found.
 * @param dim_grp Pointer that gets pointer to group info of group that contians dimension.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADDIM Dimension not found.
 * @author Ed Hartnett
 */
int
nc4_find_dim(NC_GRP_INFO_T *grp, int dimid, NC_DIM_INFO_T **dim,
             NC_GRP_INFO_T **dim_grp)
{
   NC_GRP_INFO_T *g, *dg = NULL;
   int finished = 0;

   assert(grp && dim);

   /* Find the dim info. */
   for (g = grp; g && !finished; g = g->parent)
      for ((*dim) = g->dim; (*dim); (*dim) = (*dim)->l.next)
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

/**
 * @internal Find a var (by name) in a grp.
 *
 * @param grp Pointer to group info.
 * @param name Name of var to find.
 * @param var Pointer that gets pointer to var info struct, if found.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_find_var(NC_GRP_INFO_T *grp, const char *name, NC_VAR_INFO_T **var)
{
   int i;
   assert(grp && var && name);

   /* Find the var info. */
   *var = NULL;
   for (i=0; i < grp->vars.nelems; i++)
   {
      if (0 == strcmp(name, grp->vars.value[i]->name))
      {
         *var = grp->vars.value[i];
         break;
      }
   }
   return NC_NOERR;
}

/**
 * @internal Recursively hunt for a HDF type id.
 *
 * @param start_grp Pointer to starting group info.
 * @param target_hdf_typeid HDF5 type ID to find.
 *
 * @return Pointer to type info struct, or NULL if not found.
 * @author Ed Hartnett
 */
NC_TYPE_INFO_T *
nc4_rec_find_hdf_type(NC_GRP_INFO_T *start_grp, hid_t target_hdf_typeid)
{
   NC_GRP_INFO_T *g;
   NC_TYPE_INFO_T *type, *res;
   htri_t equal;

   assert(start_grp);

   /* Does this group have the type we are searching for? */
   for (type = start_grp->type; type; type = type->l.next)
   {
      if ((equal = H5Tequal(type->native_hdf_typeid ? type->native_hdf_typeid : type->hdf_typeid, target_hdf_typeid)) < 0)
         return NULL;
      if (equal)
         return type;
   }

   /* Shake down the kids. */
   if (start_grp->children)
      for (g = start_grp->children; g; g = g->l.next)
         if ((res = nc4_rec_find_hdf_type(g, target_hdf_typeid)))
            return res;

   /* Can't find it. Fate, why do you mock me? */
   return NULL;
}

/**
 * @internal Recursively hunt for a netCDF type by name.
 *
 * @param start_grp Pointer to starting group info.
 * @param name Name of type to find.
 *
 * @return Pointer to type info, or NULL if not found.
 * @author Ed Hartnett
 */
NC_TYPE_INFO_T *
nc4_rec_find_named_type(NC_GRP_INFO_T *start_grp, char *name)
{
   NC_GRP_INFO_T *g;
   NC_TYPE_INFO_T *type, *res;

   assert(start_grp);

   /* Does this group have the type we are searching for? */
   for (type = start_grp->type; type; type = type->l.next)
      if (!strcmp(type->name, name))
         return type;

   /* Search subgroups. */
   if (start_grp->children)
      for (g = start_grp->children; g; g = g->l.next)
         if ((res = nc4_rec_find_named_type(g, name)))
            return res;

   /* Can't find it. Oh, woe is me! */
   return NULL;
}

/**
 * @internal Recursively hunt for a netCDF type id.
 *
 * @param start_grp Pointer to starting group info.
 * @param target_nc_typeid NetCDF type ID to find.
 *
 * @return Pointer to type info, or NULL if not found.
 * @author Ed Hartnett
 */
NC_TYPE_INFO_T *
nc4_rec_find_nc_type(const NC_GRP_INFO_T *start_grp, nc_type target_nc_typeid)
{
   NC_TYPE_INFO_T *type;

   assert(start_grp);

   /* Does this group have the type we are searching for? */
   for (type = start_grp->type; type; type = type->l.next)
      if (type->nc_typeid == target_nc_typeid)
         return type;

   /* Shake down the kids. */
   if (start_grp->children)
   {
      NC_GRP_INFO_T *g;

      for (g = start_grp->children; g; g = g->l.next)
      {
         NC_TYPE_INFO_T *res;

         if ((res = nc4_rec_find_nc_type(g, target_nc_typeid)))
            return res;
      }
   }

   /* Can't find it. Fate, why do you mock me? */
   return NULL;
}

/**
 * @internal Use a netCDF typeid to find a type in a type_list.
 *
 * @param h5 Pointer to HDF5 file info struct.
 * @param typeid The netCDF type ID.
 * @param type Pointer to pointer to the list of type info structs.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid input.
 * @author Ed Hartnett
 */
int
nc4_find_type(const NC_HDF5_FILE_INFO_T *h5, nc_type typeid, NC_TYPE_INFO_T **type)
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

/**
 * @internal Find the actual length of a dim by checking the length of
 * that dim in all variables that use it, in grp or children. **len
 * must be initialized to zero before this function is called.
 *
 * @param grp Pointer to group info struct.
 * @param dimid Dimension ID.
 * @param len Pointer to pointer that gets length.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_find_dim_len(NC_GRP_INFO_T *grp, int dimid, size_t **len)
{
   NC_GRP_INFO_T *g;
   NC_VAR_INFO_T *var;
   int retval;
   int i;

   assert(grp && len);
   LOG((3, "nc4_find_dim_len: grp->name %s dimid %d", grp->name, dimid));

   /* If there are any groups, call this function recursively on
    * them. */
   for (g = grp->children; g; g = g->l.next)
      if ((retval = nc4_find_dim_len(g, dimid, len)))
         return retval;

   /* For all variables in this group, find the ones that use this
    * dimension, and remember the max length. */
   for (i=0; i < grp->vars.nelems; i++)
   {
      size_t mylen;
      var = grp->vars.value[i];
      if (!var) continue;

      /* Find max length of dim in this variable... */
      if ((retval = find_var_dim_max_length(grp, var->varid, dimid, &mylen)))
         return retval;

      **len = **len > mylen ? **len : mylen;
   }

   return NC_NOERR;
}

/**
 * @internal Given a group, find an att. 
 *
 * @param grp Pointer to group info struct.
 * @param varid Variable ID.
 * @param name Name to of attribute.
 * @param attnum Number of attribute.
 * @param att Pointer to pointer that gets attribute info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOTVAR Variable not found.
 * @return ::NC_ENOTATT Attribute not found.
 * @author Ed Hartnett
 */
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
      if (varid < 0 || varid >= grp->vars.nelems)
         return NC_ENOTVAR;
      var = grp->vars.value[varid];
      if (!var) return NC_ENOTVAR;
      attlist = var->att;
      assert(var->varid == varid);
   }

   /* Now find the attribute by name or number. If a name is provided,
    * ignore the attnum. */
   if(attlist)
      for (*att = attlist; *att; *att = (*att)->l.next) {
         if (name && (*att)->name && !strcmp((*att)->name, name))
            return NC_NOERR;
         if (!name && (*att)->attnum == attnum)
            return NC_NOERR;
      }

   /* If we get here, we couldn't find the attribute. */
   return NC_ENOTATT;
}

/**
 * @internal Given an ncid, varid, and name or attnum, find and return
 * pointer to NC_ATT_INFO_T metadata.
 *
 * @param ncid File and group ID.
 * @param varid Variable ID.
 * @param name Name to of attribute.
 * @param attnum Number of attribute.
 * @param att Pointer to pointer that gets attribute info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOTVAR Variable not found.
 * @return ::NC_ENOTATT Attribute not found.
 * @author Ed Hartnett
 */
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
      if (varid < 0 || varid >= grp->vars.nelems)
         return NC_ENOTVAR;
      var = grp->vars.value[varid];
      if (!var) return NC_ENOTVAR;
      attlist = var->att;
      assert(var->varid == varid);
   }

   /* Now find the attribute by name or number. If a name is provided, ignore the attnum. */
   for (*att = attlist; *att; *att = (*att)->l.next)
      if ((name && !strcmp((*att)->name, name)) ||
          (!name && (*att)->attnum == attnum))
         return NC_NOERR;

   /* If we get here, we couldn't find the attribute. */
   return NC_ENOTATT;
}


/**
 * @internal Given an id, walk the list and find the appropriate NC.
 *
 * @param ext_ncid File/group ID to find.
 * @param h5p Pointer to pointer that gets the HDF5 file info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett, Dennis Heimbigner
 */
NC*
nc4_find_nc_file(int ext_ncid, NC_HDF5_FILE_INFO_T** h5p)
{
   NC* nc;
   int stat;

   stat = NC_check_id(ext_ncid,&nc);
   if(stat != NC_NOERR)
      nc = NULL;

   if(nc)
      if(h5p) *h5p = (NC_HDF5_FILE_INFO_T*)nc->dispatchdata;

   return nc;
}

/**
 * @internal Add object to the end of a list.
 *
 * @param list List
 * @param obj Pointer to object to add.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static void
obj_list_add(NC_LIST_NODE_T **list, NC_LIST_NODE_T *obj)
{
   /* Go to the end of the list and set the last one to point at object,
    * or, if the list is empty, our new object becomes the list. */
   if(*list)
   {
      NC_LIST_NODE_T *o;

      for (o = *list; o; o = o->next)
         if (!o->next)
            break;
      o->next = obj;
      obj->prev = o;
   }
   else
      *list = obj;
}

/**
 * @internal Remove object from a list.
 *
 * @param list List
 * @param obj Pointer to object to delete.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static void
obj_list_del(NC_LIST_NODE_T **list, NC_LIST_NODE_T *obj)
{
   /* Remove the var from the linked list. */
   if(*list == obj)
      *list = obj->next;
   else
      ((NC_LIST_NODE_T *)obj->prev)->next = obj->next;

   if(obj->next)
      ((NC_LIST_NODE_T *)obj->next)->prev = obj->prev;
}

/**
 * @internal Return a pointer to the new var.
 *
 * @param var Pointer to pointer that gets variable info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
nc4_var_add(NC_VAR_INFO_T **var)
{
   NC_VAR_INFO_T *new_var;

   /* Allocate storage for new variable. */
   if (!(new_var = calloc(1, sizeof(NC_VAR_INFO_T))))
      return NC_ENOMEM;

   /* These are the HDF5-1.8.4 defaults. */
   new_var->chunk_cache_size = nc4_chunk_cache_size;
   new_var->chunk_cache_nelems = nc4_chunk_cache_nelems;
   new_var->chunk_cache_preemption = nc4_chunk_cache_preemption;

   /* Set the var pointer, if one was given */
   if (var)
      *var = new_var;
   else
      free(new_var);

   return NC_NOERR;
}

/**
 * @internal Add to the beginning of a dim list.
 *
 * @param list List of dimension info structs.
 * @param dim Pointer to pointer that gets the new dim info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_dim_list_add(NC_DIM_INFO_T **list, NC_DIM_INFO_T **dim)
{
   NC_DIM_INFO_T *new_dim;

   if (!(new_dim = calloc(1, sizeof(NC_DIM_INFO_T))))
      return NC_ENOMEM;

   /* Add object to list */
   obj_list_add((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)new_dim);

   /* Set the dim pointer, if one was given */
   if (dim)
      *dim = new_dim;

   return NC_NOERR;
}

/**
 * @internal Add to the end of an att list.
 *
 * @param list List of att info structs.
 * @param att Pointer to pointer that gets the new att info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_att_list_add(NC_ATT_INFO_T **list, NC_ATT_INFO_T **att)
{
   NC_ATT_INFO_T *new_att;

   if (!(new_att = calloc(1, sizeof(NC_ATT_INFO_T))))
      return NC_ENOMEM;

   /* Add object to list */
   obj_list_add((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)new_att);

   /* Set the attribute pointer, if one was given */
   if (att)
      *att = new_att;

   return NC_NOERR;
}

/**
 * @internal Add to the end of a group list. Can't use 0 as a
 * new_nc_grpid - it's reserverd for the root group.
 *
 * @param list List
 * @param new_nc_grpid New group ID.
 * @param parent_grp The parent group.
 * @param nc Pointer to the file's NC struct.
 * @param name Name of the group.
 * @param grp Pointer to pointer that gets new group info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_grp_list_add(NC_GRP_INFO_T **list, int new_nc_grpid,
                 NC_GRP_INFO_T *parent_grp, NC *nc,
                 char *name, NC_GRP_INFO_T **grp)
{
   NC_GRP_INFO_T *new_grp;

   LOG((3, "%s: new_nc_grpid %d name %s ", __func__, new_nc_grpid, name));

   /* Get the memory to store this groups info. */
   if (!(new_grp = calloc(1, sizeof(NC_GRP_INFO_T))))
      return NC_ENOMEM;

   /* Fill in this group's information. */
   new_grp->nc_grpid = new_nc_grpid;
   new_grp->parent = parent_grp;
   if (!(new_grp->name = strdup(name)))
   {
      free(new_grp);
      return NC_ENOMEM;
   }
   new_grp->nc4_info = NC4_DATA(nc);

   /* Add object to list */
   obj_list_add((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)new_grp);

   /* Set the group pointer, if one was given */
   if (grp)
      *grp = new_grp;

   return NC_NOERR;
}

/**
 * @internal Names for groups, variables, and types must not be the
 * same. This function checks that a proposed name is not already in
 * use. Normalzation of UTF8 strings should happen before this
 * function is called.
 *
 * @param grp Pointer to group info struct.
 * @param name Name to check.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENAMEINUSE Name is in use.
 * @author Ed Hartnett
 */
int
nc4_check_dup_name(NC_GRP_INFO_T *grp, char *name)
{
   NC_TYPE_INFO_T *type;
   NC_GRP_INFO_T *g;
   NC_VAR_INFO_T *var;
   uint32_t hash;
   int i;

   /* Any types of this name? */
   for (type = grp->type; type; type = type->l.next)
      if (!strcmp(type->name, name))
         return NC_ENAMEINUSE;

   /* Any child groups of this name? */
   for (g = grp->children; g; g = g->l.next)
      if (!strcmp(g->name, name))
         return NC_ENAMEINUSE;

   /* Any variables of this name? */
   hash =  hash_fast(name, strlen(name));
   for (i=0; i < grp->vars.nelems; i++)
   {
      var = grp->vars.value[i];
      if (!var) continue;
      if (var->hash == hash && !strcmp(var->name, name))
         return NC_ENAMEINUSE;
   }
   return NC_NOERR;
}

/**
 * @internal Add to the end of a type list.
 *
 * @param grp Pointer to group info struct.
 * @param size Size of type in bytes.
 * @param name Name of type.
 * @param type Pointer that gets pointer to new type info
 * struct. Ignored if NULL.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
nc4_type_list_add(NC_GRP_INFO_T *grp, size_t size, const char *name,
                  NC_TYPE_INFO_T **type)
{
   NC_TYPE_INFO_T *new_type;

   /* Allocate memory for the type */
   if (!(new_type = calloc(1, sizeof(NC_TYPE_INFO_T))))
      return NC_ENOMEM;

   /* Add object to list */
   obj_list_add((NC_LIST_NODE_T **)(&grp->type), (NC_LIST_NODE_T *)new_type);

   /* Remember info about this type. */
   new_type->nc_typeid = grp->nc4_info->next_typeid++;
   new_type->size = size;
   if (!(new_type->name = strdup(name)))
      return NC_ENOMEM;

   /* Increment the ref. count on the type */
   new_type->rc++;

   /* Return a pointer to the new type, if requested */
   if (type)
      *type = new_type;

   return NC_NOERR;
}

/**
 * @internal Add to the end of a compound field list.
 *
 * @param list Pointer to pointer of list of field info structs.
 * @param fieldid The ID of this field.
 * @param name Name of the field.
 * @param offset Offset in bytes.
 * @param field_hdf_typeid The HDF5 type ID of the field.
 * @param native_typeid The HDF5 native type ID of the field.
 * @param xtype The netCDF type of the field.
 * @param ndims The number of dimensions of the field.
 * @param dim_sizesp An array of dim sizes for the field.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_field_list_add(NC_FIELD_INFO_T **list, int fieldid, const char *name,
                   size_t offset, hid_t field_hdf_typeid, hid_t native_typeid,
                   nc_type xtype, int ndims, const int *dim_sizesp)
{
   NC_FIELD_INFO_T *field;

   /* Name has already been checked and UTF8 normalized. */
   if (!name)
      return NC_EINVAL;

   /* Allocate storage for this field information. */
   if (!(field = calloc(1, sizeof(NC_FIELD_INFO_T))))
      return NC_ENOMEM;

   /* Store the information about this field. */
   field->fieldid = fieldid;
   if (!(field->name = strdup(name)))
   {
      free(field);
      return NC_ENOMEM;
   }
   field->hdf_typeid = field_hdf_typeid;
   field->native_hdf_typeid = native_typeid;
   field->nc_typeid = xtype;
   field->offset = offset;
   field->ndims = ndims;
   if (ndims)
   {
      int i;

      if (!(field->dim_size = malloc(ndims * sizeof(int))))
      {
         free(field->name);
         free(field);
         return NC_ENOMEM;
      }
      for (i = 0; i < ndims; i++)
         field->dim_size[i] = dim_sizesp[i];
   }

   /* Add object to list */
   obj_list_add((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)field);

   return NC_NOERR;
}

/**
 * @internal Add a member to an enum type.
 *
 * @param list Pointer to pointer of list of enum member info structs.
 * @param size Size in bytes of new member.
 * @param name Name of the member.
 * @param value Value to associate with member.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
nc4_enum_member_add(NC_ENUM_MEMBER_INFO_T **list, size_t size,
                    const char *name, const void *value)
{
   NC_ENUM_MEMBER_INFO_T *member;

   /* Name has already been checked. */
   assert(name && size > 0 && value);
   LOG((4, "%s: size %d name %s", __func__, size, name));

   /* Allocate storage for this field information. */
   if (!(member = calloc(1, sizeof(NC_ENUM_MEMBER_INFO_T))))
      return NC_ENOMEM;
   if (!(member->value = malloc(size))) {
      free(member);
      return NC_ENOMEM;
   }
   if (!(member->name = strdup(name))) {
      free(member->value);
      free(member);
      return NC_ENOMEM;
   }

   /* Store the value for this member. */
   memcpy(member->value, value, size);

   /* Add object to list */
   obj_list_add((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)member);

   return NC_NOERR;
}

/**
 * @internal Delete a field from a field list, and nc_free the memory.
 *
 * @param list Pointer to pointer of list of field info structs.
 * @param field Pointer to field info of field to delete.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static void
field_list_del(NC_FIELD_INFO_T **list, NC_FIELD_INFO_T *field)
{
   /* Take this field out of the list. */
   obj_list_del((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)field);

   /* Free some stuff. */
   if (field->name)
      free(field->name);
   if (field->dim_size)
      free(field->dim_size);

   /* Nc_Free the memory. */
   free(field);
}

/**
 * @internal Free allocated space for type information.
 *
 * @param type Pointer to type info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_type_free(NC_TYPE_INFO_T *type)
{
   /* Decrement the ref. count on the type */
   assert(type->rc);
   type->rc--;

   /* Release the type, if the ref. count drops to zero */
   if (0 == type->rc)
   {
      /* Close any open user-defined HDF5 typeids. */
      if (type->hdf_typeid && H5Tclose(type->hdf_typeid) < 0)
         return NC_EHDFERR;
      if (type->native_hdf_typeid && H5Tclose(type->native_hdf_typeid) < 0)
         return NC_EHDFERR;

      /* Free the name. */
      if (type->name)
         free(type->name);

      /* Class-specific cleanup */
      switch (type->nc_type_class)
      {
      case NC_COMPOUND:
      {
         NC_FIELD_INFO_T *field;

         /* Delete all the fields in this type (there will be some if its a
          * compound). */
         field = type->u.c.field;
         while (field)
         {
            NC_FIELD_INFO_T *f = field->l.next;

            field_list_del(&type->u.c.field, field);
            field = f;
         }
      }
      break;

      case NC_ENUM:
      {
         NC_ENUM_MEMBER_INFO_T *enum_member;

         /* Delete all the enum_members, if any. */
         enum_member = type->u.e.enum_member;
         while (enum_member)
         {
            NC_ENUM_MEMBER_INFO_T *em = enum_member->l.next;

            free(enum_member->value);
            free(enum_member->name);
            free(enum_member);
            enum_member = em;
         }

         if (H5Tclose(type->u.e.base_hdf_typeid) < 0)
            return NC_EHDFERR;
      }
      break;

      case NC_VLEN:
         if (H5Tclose(type->u.v.base_hdf_typeid) < 0)
            return NC_EHDFERR;

      default:
         break;
      }

      /* Release the memory. */
      free(type);
   }

   return NC_NOERR;
}

/**
 * @internal  Delete a var, and free the memory.
 *
 * @param var Pointer to the var info struct of var to delete.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_var_del(NC_VAR_INFO_T *var)
{
   NC_ATT_INFO_T *a, *att;
   int ret;

   if(var == NULL)
      return NC_NOERR;

   /* First delete all the attributes attached to this var. */
   att = var->att;
   while (att)
   {
      a = att->l.next;
      if ((ret = nc4_att_list_del(&var->att, att)))
         return ret;
      att = a;
   }

   /* Free some things that may be allocated. */
   if (var->chunksizes)
   {free(var->chunksizes);var->chunksizes = NULL;}

   if (var->hdf5_name)
   {free(var->hdf5_name); var->hdf5_name = NULL;}

   if (var->name)
   {free(var->name); var->name = NULL;}

   if (var->dimids)
   {free(var->dimids); var->dimids = NULL;}

   if (var->dim)
   {free(var->dim); var->dim = NULL;}

   /* Delete any fill value allocation. This must be done before the
    * type_info is freed. */
   if (var->fill_value)
   {
      if (var->hdf_datasetid)
      {
         if (var->type_info)
         {
            if (var->type_info->nc_type_class == NC_VLEN)
               nc_free_vlen((nc_vlen_t *)var->fill_value);
            else if (var->type_info->nc_type_class == NC_STRING && *(char **)var->fill_value)
               free(*(char **)var->fill_value);
         }
      }
      free(var->fill_value);
      var->fill_value = NULL;
   }

   /* Release type information */
   if (var->type_info)
   {
      int retval;

      if ((retval = nc4_type_free(var->type_info)))
         return retval;
      var->type_info = NULL;
   }

   /* Delete any HDF5 dimscale objid information. */
   if (var->dimscale_hdf5_objids)
      free(var->dimscale_hdf5_objids);

   /* Delete information about the attachment status of dimscales. */
   if (var->dimscale_attached)
      free(var->dimscale_attached);

   /* Release parameter information. */
   if (var->params)
      free(var->params);

   /* Delete the var. */
   free(var);

   return NC_NOERR;
}

/**
 * @internal Delete a type from a type list, and nc_free the memory.
 *
 * @param list Pointer to pointer of list of types.
 * @param type Pointer to type info struct of type to delete.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
type_list_del(NC_TYPE_INFO_T **list, NC_TYPE_INFO_T *type)
{
   /* Take this type out of the list. */
   obj_list_del((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)type);

   /* Free the type, and its components */
   return nc4_type_free(type);
}

/**
 * @internal Delete a del from a var list, and nc_free the memory.
 *
 * @param list Pointer to pointer of list of dims.
 * @param dim Pointer to dim info struct of type to delete.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_dim_list_del(NC_DIM_INFO_T **list, NC_DIM_INFO_T *dim)
{
   /* Take this dimension out of the list. */
   obj_list_del((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)dim);

   /* Free memory allocated for names. */
   if (dim->name)
      free(dim->name);

   free(dim);
   return NC_NOERR;
}

/**
 * @internal Remove a NC_GRP_INFO_T from the linked list. This will
 * nc_free the memory too.
 *
 * @param list Pointer to pointer of list of group info structs.
 * @param grp Pointer to group info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static void
grp_list_del(NC_GRP_INFO_T **list, NC_GRP_INFO_T *grp)
{
   /* Take this group out of the list. */
   obj_list_del((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)grp);

   free(grp);
}

/**
 * @internal Recursively delete the data for a group (and everything
 * it contains) in our internal metadata store.
 *
 * @param list Pointer to pointer of list.
 * @param grp Pointer to group info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_rec_grp_del(NC_GRP_INFO_T **list, NC_GRP_INFO_T *grp)
{
   NC_GRP_INFO_T *g, *c;
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T *a, *att;
   NC_DIM_INFO_T *d, *dim;
   NC_TYPE_INFO_T *type, *t;
   int retval;
   int i;

   assert(grp);
   LOG((3, "%s: grp->name %s", __func__, grp->name));

   /* Recursively call this function for each child, if any, stopping
    * if there is an error. */
   g = grp->children;
   while(g)
   {
      c = g->l.next;
      if ((retval = nc4_rec_grp_del(&(grp->children), g)))
         return retval;
      g = c;
   }

   /* Delete all the list contents for vars, dims, and atts, in each
    * group. */
   att = grp->att;
   while (att)
   {
      LOG((4, "%s: deleting att %s", __func__, att->name));
      a = att->l.next;
      if ((retval = nc4_att_list_del(&grp->att, att)))
         return retval;
      att = a;
   }

   /* Delete all vars. */
   for (i=0; i < grp->vars.nelems; i++)
   {
      var = grp->vars.value[i];
      if (!var) continue;

      LOG((4, "%s: deleting var %s", __func__, var->name));
      /* Close HDF5 dataset associated with this var, unless it's a
       * scale. */
      if (var->hdf_datasetid && H5Dclose(var->hdf_datasetid) < 0)
         return NC_EHDFERR;
      if ((retval = nc4_var_del(var)))
         return retval;
      grp->vars.value[i] = NULL;
   }

   /* Vars are all freed above.  When eliminate linked-list,
      then need to iterate value and free vars from it.
   */
   if (grp->vars.nalloc != 0) {
      assert(grp->vars.value != NULL);
      free(grp->vars.value);
      grp->vars.value = NULL;
      grp->vars.nalloc = 0;
   }

   /* Delete all dims. */
   dim = grp->dim;
   while (dim)
   {
      LOG((4, "%s: deleting dim %s", __func__, dim->name));
      /* If this is a dim without a coordinate variable, then close
       * the HDF5 DIM_WITHOUT_VARIABLE dataset associated with this
       * dim. */
      if (dim->hdf_dimscaleid && H5Dclose(dim->hdf_dimscaleid) < 0)
         return NC_EHDFERR;
      d = dim->l.next;
      if ((retval = nc4_dim_list_del(&grp->dim, dim)))
         return retval;
      dim = d;
   }

   /* Delete all types. */
   type = grp->type;
   while (type)
   {
      LOG((4, "%s: deleting type %s", __func__, type->name));
      t = type->l.next;
      if ((retval = type_list_del(&grp->type, type)))
         return retval;
      type = t;
   }

   /* Tell HDF5 we're closing this group. */
   LOG((4, "%s: closing group %s", __func__, grp->name));
   if (grp->hdf_grpid && H5Gclose(grp->hdf_grpid) < 0)
      return NC_EHDFERR;

   /* Free the name. */
   free(grp->name);

   /* Finally, redirect pointers around this entry in the list, and
    * nc_free its memory. */
   grp_list_del(list, grp);

   return NC_NOERR;
}

/**
 * @internal Remove a NC_ATT_INFO_T from the linked list. This will
 * nc_free the memory too.
 *
 * @param list Pointer to pointer of list.
 * @param att Pointer to attribute info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc4_att_list_del(NC_ATT_INFO_T **list, NC_ATT_INFO_T *att)
{
   int i;

   /* Take this att out of the list. */
   obj_list_del((NC_LIST_NODE_T **)list, (NC_LIST_NODE_T *)att);

   /* Free memory that was malloced to hold data for this
    * attribute. */
   if (att->data)
      free(att->data);

   /* Free the name. */
   if (att->name)
      free(att->name);

   /* Close the HDF5 typeid. */
   if (att->native_hdf_typeid && H5Tclose(att->native_hdf_typeid) < 0)
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
         if(att->stdata[i])
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

/**
 * @internal Break a coordinate variable to separate the dimension and
 * the variable.
 *
 * This is called from nc_rename_dim() and nc_rename_var(). In some
 * renames, the coord variable must stay, but it is no longer a coord
 * variable. This function changes a coord var into an ordinary
 * variable.
 *
 * @param grp Pointer to group info struct.
 * @param coord_var Pointer to variable info struct.
 * @param dim Pointer to dimension info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Quincey Koziol, Ed Hartnett
 */
int
nc4_break_coord_var(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *coord_var, NC_DIM_INFO_T *dim)
{
   int retval = NC_NOERR;

   /* Sanity checks */
   assert(grp && coord_var && dim && dim->coord_var == coord_var &&
          coord_var->dim[0] == dim && coord_var->dimids[0] == dim->dimid &&
          !dim->hdf_dimscaleid);
   LOG((3, "%s dim %s was associated with var %s, but now has different name",
        __func__, dim->name, coord_var->name));

   /* If we're replacing an existing dimscale dataset, go to
    * every var in the file and detach this dimension scale. */
   if ((retval = rec_detach_scales(grp->nc4_info->root_grp,
                                   dim->dimid, coord_var->hdf_datasetid)))
      return retval;

   /* Allow attached dimscales to be tracked on the [former]
    * coordinate variable */
   if (coord_var->ndims)
   {
      /* Coordinate variables shouldn't have dimscales attached. */
      assert(!coord_var->dimscale_attached);

      /* Allocate space for tracking them */
      if (!(coord_var->dimscale_attached = calloc(coord_var->ndims,
                                                  sizeof(nc_bool_t))))
         return NC_ENOMEM;
   }

   /* Remove the atts that go with being a coordinate var. */
   /* if ((retval = remove_coord_atts(coord_var->hdf_datasetid))) */
   /*    return retval; */

   /* Detach dimension from variable */
   coord_var->dimscale = NC_FALSE;
   dim->coord_var = NULL;

   /* Set state transition indicators */
   coord_var->was_coord_var = NC_TRUE;
   coord_var->became_coord_var = NC_FALSE;

   return NC_NOERR;
}

/**
 * @internal Delete an existing dimscale-only dataset.
 *
 * A dimscale-only HDF5 dataset is created when a dim is defined
 * without an accompanying coordinate variable.
 *
 * Sometimes, during renames, or late creation of variables, an
 * existing, dimscale-only dataset must be removed. This means
 * detatching all variables that use the dataset, then closing and
 * unlinking it.
 *
 * @param grp The grp of the dimscale-only dataset to be deleted, or a
 * higher group in the heirarchy (ex. root group).
 * @param dim Pointer to the dim with the dimscale-only dataset to be
 * deleted.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EHDFERR HDF5 error.
 * @author Ed Hartnett
 */
int
delete_existing_dimscale_dataset(NC_GRP_INFO_T *grp, int dimid, NC_DIM_INFO_T *dim)
{
   int retval;

   assert(grp && dim);
   LOG((2, "%s: deleting dimscale dataset %s dimid %d", __func__, dim->name,
        dimid));

   /* Detach dimscale from any variables using it */
   if ((retval = rec_detach_scales(grp, dimid, dim->hdf_dimscaleid)) < 0)
      return retval;

   /* Close the HDF5 dataset */
   if (H5Dclose(dim->hdf_dimscaleid) < 0)
      return NC_EHDFERR;
   dim->hdf_dimscaleid = 0;

   /* Now delete the dataset. */
   if (H5Gunlink(grp->hdf_grpid, dim->name) < 0)
      return NC_EHDFERR;

   return NC_NOERR;
}

/**
 * @internal Reform a coordinate variable from a dimension and a
 * variable.
 *
 * @param grp Pointer to group info struct.
 * @param var Pointer to variable info struct.
 * @param dim Pointer to dimension info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Quincey Koziol
 */
int
nc4_reform_coord_var(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var, NC_DIM_INFO_T *dim)
{
   int need_to_reattach_scales = 0;
   int retval = NC_NOERR;

   assert(grp && var && dim);
   LOG((3, "%s: dim->name %s var->name %s", __func__, dim->name, var->name));

   /* Detach dimscales from the [new] coordinate variable */
   if(var->dimscale_attached)
   {
      int dims_detached = 0;
      int finished = 0;
      int d;

      /* Loop over all dimensions for variable */
      for (d = 0; d < var->ndims && !finished; d++)
      {
         /* Is there a dimscale attached to this axis? */
         if(var->dimscale_attached[d])
         {
            NC_GRP_INFO_T *g;

            for (g = grp; g && !finished; g = g->parent)
            {
               NC_DIM_INFO_T *dim1;

               for (dim1 = g->dim; dim1 && !finished; dim1 = dim1->l.next)
               {
                  if (var->dimids[d] == dim1->dimid)
                  {
                     hid_t dim_datasetid;  /* Dataset ID for dimension */

                     /* Find dataset ID for dimension */
                     if (dim1->coord_var)
                        dim_datasetid = dim1->coord_var->hdf_datasetid;
                     else
                        dim_datasetid = dim1->hdf_dimscaleid;

                     /* dim_datasetid may be 0 in some cases when
                      * renames of dims and vars are happening. In
                      * this case, the scale has already been
                      * detached. */
                     if (dim_datasetid > 0)
                     {
                        LOG((3, "detaching scale from %s", var->name));
                        if (H5DSdetach_scale(var->hdf_datasetid, dim_datasetid, d) < 0)
                           BAIL(NC_EHDFERR);
                     }
                     var->dimscale_attached[d] = NC_FALSE;
                     if (dims_detached++ == var->ndims)
                        finished++;
                  }
               }
            }
         }
      } /* next variable dimension */

      /* Release & reset the array tracking attached dimscales */
      free(var->dimscale_attached);
      var->dimscale_attached = NULL;
      need_to_reattach_scales++;
   }

   /* Use variable's dataset ID for the dimscale ID. */
   if (dim->hdf_dimscaleid && grp != NULL)
   {
      LOG((3, "closing and unlinking dimscale dataset %s", dim->name));
      if (H5Dclose(dim->hdf_dimscaleid) < 0)
         BAIL(NC_EHDFERR);
      dim->hdf_dimscaleid = 0;

      /* Now delete the dimscale's dataset
         (it will be recreated later, if necessary) */
      if (H5Gunlink(grp->hdf_grpid, dim->name) < 0)
         return NC_EDIMMETA;
   }

   /* Attach variable to dimension */
   var->dimscale = NC_TRUE;
   dim->coord_var = var;

   /* Check if this variable used to be a coord. var */
   if (need_to_reattach_scales || (var->was_coord_var && grp != NULL))
   {
      /* Reattach the scale everywhere it is used. */
      /* (Recall that netCDF dimscales are always 1-D) */
      if ((retval = rec_reattach_scales(grp->nc4_info->root_grp,
                                        var->dimids[0], var->hdf_datasetid)))
         return retval;

      /* Set state transition indicator (cancels earlier transition) */
      var->was_coord_var = NC_FALSE;
   }
   else
      /* Set state transition indicator */
      var->became_coord_var = NC_TRUE;

exit:
   return retval;
}

/**
 * @internal Normalize a UTF8 name. Put the result in norm_name, which
 * can be NC_MAX_NAME + 1 in size. This function makes sure the free()
 * gets called on the return from utf8proc_NFC, and also ensures that
 * the name is not too long.
 *
 * @param name Name to normalize.
 * @param norm_name The normalized name.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EMAXNAME Name too long.
 * @author Dennis Heimbigner
 */
int
nc4_normalize_name(const char *name, char *norm_name)
{
   char *temp_name;
   int stat = nc_utf8_normalize((const unsigned char *)name,(unsigned char **)&temp_name);
   if(stat != NC_NOERR)
      return stat;
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
/**
 * Use this to set the global log level. Set it to NC_TURN_OFF_LOGGING
 * (-1) to turn off all logging. Set it to 0 to show only errors, and
 * to higher numbers to show more and more logging details.
 *
 * @param new_level The new logging level.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
nc_set_log_level(int new_level)
{
   if(!nc4_hdf5_initialized)
      nc4_hdf5_initialize();

   /* If the user wants to completely turn off logging, turn off HDF5
      logging too. Now I truely can't think of what to do if this
      fails, so just ignore the return code. */
   if (new_level == NC_TURN_OFF_LOGGING)
   {
      set_auto(NULL,NULL);
      LOG((1, "HDF5 error messages turned off!"));
   }

   /* Do we need to turn HDF5 logging back on? */
   if (new_level > NC_TURN_OFF_LOGGING &&
       nc_log_level <= NC_TURN_OFF_LOGGING)
   {
      if (set_auto((H5E_auto_t)&H5Eprint, stderr) < 0)
         LOG((0, "H5Eset_auto failed!"));
      LOG((1, "HDF5 error messages turned on."));
   }

   /* Now remember the new level. */
   nc_log_level = new_level;
   LOG((4, "log_level changed to %d", nc_log_level));
   return 0;
}

#define MAX_NESTS 10
/**
 * @internal Recursively print the metadata of a group.
 *
 * @param grp Pointer to group info struct.
 * @param tab_count Number of tabs.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
rec_print_metadata(NC_GRP_INFO_T *grp, int tab_count)
{
   NC_GRP_INFO_T *g;
   NC_ATT_INFO_T *att;
   NC_VAR_INFO_T *var;
   NC_DIM_INFO_T *dim;
   NC_TYPE_INFO_T *type;
   NC_FIELD_INFO_T *field;
   char tabs[MAX_NESTS+1] = "";
   char *dims_string = NULL;
   char temp_string[10];
   int t, retval, d, i;

   /* Come up with a number of tabs relative to the group. */
   for (t = 0; t < tab_count && t < MAX_NESTS; t++)
      tabs[t] = '\t';
   tabs[t] = '\0';

   LOG((2, "%s GROUP - %s nc_grpid: %d nvars: %d natts: %d",
        tabs, grp->name, grp->nc_grpid, grp->nvars, grp->natts));

   for(att = grp->att; att; att = att->l.next)
      LOG((2, "%s GROUP ATTRIBUTE - attnum: %d name: %s type: %d len: %d",
           tabs, att->attnum, att->name, att->nc_typeid, att->len));

   for(dim = grp->dim; dim; dim = dim->l.next)
      LOG((2, "%s DIMENSION - dimid: %d name: %s len: %d unlimited: %d",
           tabs, dim->dimid, dim->name, dim->len, dim->unlimited));

   for (i=0; i < grp->vars.nelems; i++)
   {
      var = grp->vars.value[i];
      if (!var) continue;
      if(var->ndims > 0)
      {
         dims_string = (char*)malloc(sizeof(char)*(var->ndims*4));
         strcpy(dims_string, "");
         for (d = 0; d < var->ndims; d++)
         {
            sprintf(temp_string, " %d", var->dimids[d]);
            strcat(dims_string, temp_string);
         }
      }
      LOG((2, "%s VARIABLE - varid: %d name: %s type: %d ndims: %d dimscale: %d dimids:%s endianness: %d, hdf_typeid: %d",
           tabs, var->varid, var->name, var->type_info->nc_typeid, var->ndims, (int)var->dimscale,
           (dims_string ? dims_string : " -"),var->type_info->endianness, var->type_info->native_hdf_typeid));
      for(att = var->att; att; att = att->l.next)
         LOG((2, "%s VAR ATTRIBUTE - attnum: %d name: %s type: %d len: %d",
              tabs, att->attnum, att->name, att->nc_typeid, att->len));
      if(dims_string)
      {
         free(dims_string);
         dims_string = NULL;
      }
   }

   for (type = grp->type; type; type = type->l.next)
   {
      LOG((2, "%s TYPE - nc_typeid: %d hdf_typeid: 0x%x size: %d committed: %d "
           "name: %s num_fields: %d", tabs, type->nc_typeid,
           type->hdf_typeid, type->size, (int)type->committed, type->name,
           type->u.c.num_fields));
      /* Is this a compound type? */
      if (type->nc_type_class == NC_COMPOUND)
      {
         LOG((3, "compound type"));
         for (field = type->u.c.field; field; field = field->l.next)
            LOG((4, "field %s offset %d nctype %d ndims %d", field->name,
                 field->offset, field->nc_typeid, field->ndims));
      }
      else if (type->nc_type_class == NC_VLEN)
      {
         LOG((3, "VLEN type"));
         LOG((4, "base_nc_type: %d", type->u.v.base_nc_typeid));
      }
      else if (type->nc_type_class == NC_OPAQUE)
         LOG((3, "Opaque type"));
      else if (type->nc_type_class == NC_ENUM)
      {
         LOG((3, "Enum type"));
         LOG((4, "base_nc_type: %d", type->u.e.base_nc_typeid));
      }
      else
      {
         LOG((0, "Unknown class: %d", type->nc_type_class));
         return NC_EBADTYPE;
      }
   }

   /* Call self for each child of this group. */
   if (grp->children)
   {
      for (g = grp->children; g; g = g->l.next)
         if ((retval = rec_print_metadata(g, tab_count + 1)))
            return retval;
   }

   return NC_NOERR;
}

/**
 * @internal Print out the internal metadata for a file. This is
 * useful to check that netCDF is working! Nonetheless, this function
 * will print nothing if logging is not set to at least two.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
log_metadata_nc(NC *nc)
{
   NC_HDF5_FILE_INFO_T *h5 = NC4_DATA(nc);

   LOG((2, "*** NetCDF-4 Internal Metadata: int_ncid 0x%x ext_ncid 0x%x",
        nc->int_ncid, nc->ext_ncid));
   if (!h5)
   {
      LOG((2, "This is a netCDF-3 file."));
      return NC_NOERR;
   }
   LOG((2, "FILE - hdfid: 0x%x path: %s cmode: 0x%x parallel: %d redef: %d "
        "fill_mode: %d no_write: %d next_nc_grpid: %d", h5->hdfid, nc->path,
        h5->cmode, (int)h5->parallel, (int)h5->redef, h5->fill_mode, (int)h5->no_write,
        h5->next_nc_grpid));
   return rec_print_metadata(h5->root_grp, 0);
}

#endif /*LOGGING */

/**
 * @internal Show the in-memory metadata for a netcdf file.
 *
 * @param ncid File and group ID.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
int
NC4_show_metadata(int ncid)
{
   int retval = NC_NOERR;
#ifdef LOGGING
   NC *nc;
   int old_log_level = nc_log_level;

   /* Find file metadata. */
   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   /* Log level must be 2 to see metadata. */
   nc_log_level = 2;
   retval = log_metadata_nc(nc);
   nc_log_level = old_log_level;
#endif /*LOGGING*/
   return retval;
}
