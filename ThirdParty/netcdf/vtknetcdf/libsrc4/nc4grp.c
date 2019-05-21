/*
This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc4 groups.

Copyright 2005, University Corporation for Atmospheric Research. See
netcdf-4/docs/COPYRIGHT file for copying and redistribution
conditions.

$Id: nc4grp.c,v 1.44 2010/05/25 17:54:23 dmh Exp $
*/

#include "nc4internal.h"
#include "nc4dispatch.h"

/* Create a group. It's ncid is returned in the new_ncid pointer. */
int
NC4_def_grp(int parent_ncid, const char *name, int *new_ncid)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   LOG((2, "%s: parent_ncid 0x%x name %s", __func__, parent_ncid, name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(parent_ncid, &grp, &h5)))
      return retval;
   if (!h5)
      return NC_ENOTNC4;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* Check that this name is not in use as a var, grp, or type. */
   if ((retval = nc4_check_dup_name(grp, norm_name)))
      return retval;

   /* No groups in netcdf-3! */
   if (h5->cmode & NC_CLASSIC_MODEL)
      return NC_ESTRICTNC3;

   /* If it's not in define mode, switch to define mode. */
   if (!(h5->flags & NC_INDEF))
      if ((retval = NC4_redef(parent_ncid)))
	 return retval;

   /* Update internal lists to reflect new group. The actual HDF5
    * group creation will be done when metadata is written by a
    * sync. */
   if ((retval = nc4_grp_list_add(&(grp->children), h5->next_nc_grpid, 
				  grp, grp->nc4_info->controller, norm_name, NULL)))
      return retval;
   if (new_ncid)
      *new_ncid = grp->nc4_info->controller->ext_ncid | h5->next_nc_grpid;
   h5->next_nc_grpid++;
   
   return NC_NOERR;
}

/* Rename a group. */
int
NC4_rename_grp(int grpid, const char *name)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   LOG((2, "nc_rename_grp: grpid 0x%x name %s", grpid, name));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(grpid, &grp, &h5)))
      return retval;
   if (!h5)
      return NC_ENOTNC4;

   if (h5->no_write)
      return NC_EPERM; /* attempt to write to a read-only file */

   /* Do not allow renaming the root group */
   if(grp->parent == NULL)
	return NC_EBADGRPID;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* Check that this name is not in use as a var, grp, or type. */
   if ((retval = nc4_check_dup_name(grp, norm_name)))
      return retval;

   /* If it's not in define mode, switch to define mode. */
   if (!(h5->flags & NC_INDEF))
      if ((retval = NC4_redef(grpid)))
	 return retval;

   /* Rename the group, if it exists in the file */
   if (grp->hdf_grpid)
   {
      /* Close the group */
      if (H5Gclose(grp->hdf_grpid) < 0)
         return NC_EHDFERR;
      grp->hdf_grpid = 0;

      /* Attempt to rename & re-open the group, if the parent group is open */
      if (grp->parent->hdf_grpid)
      {
         /* Rename the group */
         if (H5Gmove(grp->parent->hdf_grpid, grp->name, name) < 0)
            return NC_EHDFERR;

         /* Reopen the group, with the new name */
         if ((grp->hdf_grpid = H5Gopen2(grp->parent->hdf_grpid, name, H5P_DEFAULT)) < 0)
            return NC_EHDFERR;
      }
   }

   /* Give the group its new name in metadata. UTF8 normalization
    * has been done. */
   free(grp->name);
   if (!(grp->name = malloc((strlen(norm_name) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(grp->name, norm_name);

   return NC_NOERR;
}

/* Given an ncid and group name (NULL gets root group), return
 * the ncid of that group. */
int
NC4_inq_ncid(int ncid, const char *name, int *grp_ncid)
{
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   char norm_name[NC_MAX_NAME + 1];
   int retval;

   LOG((2, "nc_inq_ncid: ncid 0x%x name %s", ncid, name));
   
   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   /* Groups only work with netCDF-4/HDF5 files... */
   if (!h5)
      return NC_ENOTNC4;

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   /* Look through groups for one of this name. */
   for (g = grp->children; g; g = g->l.next)
      if (!strcmp(norm_name, g->name)) /* found it! */
      {
	 if (grp_ncid)
	    *grp_ncid = grp->nc4_info->controller->ext_ncid | g->nc_grpid;
	 return NC_NOERR;
      }
   
   /* If we got here, we didn't find the named group. */
   return NC_ENOGRP;
}

/* Given a location id, return the number of groups it contains, and
 * an array of their locids. */
int
NC4_inq_grps(int ncid, int *numgrps, int *ncids)
{
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   int num = 0;
   int retval;

   LOG((2, "nc_inq_grps: ncid 0x%x", ncid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   /* For netCDF-3 files, just report zero groups. */
   if (!h5)
   {
      if (numgrps)
	 *numgrps = 0;
      return NC_NOERR;
   }

   /* Count the number of groups in this group. */
   for (g = grp->children; g; g = g->l.next)
   {
      if (ncids)
      {
	 /* Combine the nc_grpid in a bitwise or with the ext_ncid,
	  * which allows the returned ncid to carry both file and
	  * group information. */
	 *ncids = g->nc_grpid | g->nc4_info->controller->ext_ncid;
	 ncids++;
      }
      num++;
   }
   
   if (numgrps)
      *numgrps = num;

   return NC_NOERR;
}

/* Given locid, find name of group. (Root group is named "/".) */
int
NC4_inq_grpname(int ncid, char *name)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   int retval;

   LOG((2, "nc_inq_grpname: ncid 0x%x", ncid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;
   if (name)
   {
      if (!h5)
	 strcpy(name, "/");
      else
	 strcpy(name, grp->name);
   }

   return NC_NOERR;
}

/* Find the full path name to the group represented by ncid. Either
 * pointer argument may be NULL; pass a NULL for the third parameter
 * to get the length of the full path name. The length will not
 * include room for a null pointer. */
int
NC4_inq_grpname_full(int ncid, size_t *lenp, char *full_name)
{
   char *name, grp_name[NC_MAX_NAME + 1];
   int g, id = ncid, parent_id, *gid;
   int i, ret = NC_NOERR;

   /* How many generations? */
   for (g = 0; !NC4_inq_grp_parent(id, &parent_id); g++, id = parent_id)
      ;

   /* Allocate storage. */
   if (!(name = malloc((g + 1) * (NC_MAX_NAME + 1) + 1)))
      return NC_ENOMEM;
   if (!(gid = malloc((g + 1) * sizeof(int))))
   {
      free(name);
      return NC_ENOMEM;
   }
   assert(name && gid);

   /* Always start with a "/" for the root group. */
   strcpy(name, "/");

   /* Get the ncids for all generations. */
   gid[0] = ncid;
   for (i = 1; i < g && !ret; i++)
      ret = NC4_inq_grp_parent(gid[i - 1], &gid[i]);

   /* Assemble the full name. */
   for (i = g - 1; !ret && i >= 0 && !ret; i--)
   {
      if ((ret = NC4_inq_grpname(gid[i], grp_name)))
	 break;
      strcat(name, grp_name);
      if (i)
	 strcat(name, "/");
   }

   /* Give the user the length of the name, if he wants it. */
   if (!ret && lenp)
      *lenp = strlen(name);

   /* Give the user the name, if he wants it. */
   if (!ret && full_name)
      strcpy(full_name, name);

   free(gid);
   free(name);

   return ret;
}

/* Find the parent ncid of a group. For the root group, return
 * NC_ENOGRP error.  *Now* I know what kind of tinfoil hat wearing nut
 * job would call this function with a NULL pointer for parent_ncid -
 * Russ Rew!! */
int
NC4_inq_grp_parent(int ncid, int *parent_ncid)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   int retval;

   LOG((2, "nc_inq_grp_parent: ncid 0x%x", ncid));

   /* Find info for this file and group. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   /* Groups only work with netCDF-4/HDF5 files... */
   if (!h5)
      return NC_ENOGRP;

   /* Set the parent ncid, if there is one. */
   if (grp->parent)
   {
      if (parent_ncid)
	 *parent_ncid = grp->nc4_info->controller->ext_ncid | grp->parent->nc_grpid;
   }
   else
      return NC_ENOGRP;
   
   return NC_NOERR;
}

/* Given a full name and ncid, find group ncid. */
int
NC4_inq_grp_full_ncid(int ncid, const char *full_name, int *grp_ncid)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   int id1 = ncid, id2;
   char *cp, *full_name_cpy;
   int ret;

   if (!full_name)
      return NC_EINVAL;

   /* Find info for this file and group, and set pointer to each. */
   if ((ret = nc4_find_grp_h5(ncid, &grp, &h5)))
      return ret;

   /* Copy full_name because strtok messes with the value it works
    * with, and we don't want to mess up full_name. */
   if (!(full_name_cpy = malloc(strlen(full_name) + 1)))
      return NC_ENOMEM;
   strcpy(full_name_cpy, full_name);

   /* Get the first part of the name. */
   if (!(cp = strtok(full_name_cpy, "/")))
   {
      /* If "/" is passed, and this is the root group, return the root
       * group id. */
      if (!grp->parent)
	 id2 = ncid;
      else
      {
	 free(full_name_cpy);
	 return NC_ENOGRP;
      }
   }
   else
   {
      /* Keep parsing the string. */
      for (; cp; id1 = id2)
      {
	 if ((ret = NC4_inq_ncid(id1, cp, &id2)))
	 {
	    free(full_name_cpy);
	    return ret;
	 }
	 cp = strtok(NULL, "/");
      }
   }

   /* Give the user the requested value. */
   if (grp_ncid)
      *grp_ncid = id2;

   free(full_name_cpy);

   return NC_NOERR;
}

/* Get a list of ids for all the variables in a group. */
int
NC4_inq_varids(int ncid, int *nvars, int *varids)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   int v, num_vars = 0;
   int retval;
   int i;

   LOG((2, "nc_inq_varids: ncid 0x%x", ncid));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   if (!h5)
   {
      /* If this is a netcdf-3 file, there is only one group, the root
       * group, and its vars have ids 0 thru nvars - 1. */
      if ((retval = NC4_inq(ncid, NULL, &num_vars, NULL, NULL)))
	 return retval;
      if (varids)
	 for (v = 0; v < num_vars; v++)
	    varids[v] = v;
   }
   else
   {
      /* This is a netCDF-4 group. Round up them doggies and count
       * 'em. The list is in correct (i.e. creation) order. */
      for (i=0; i < grp->vars.nelems; i++)
      {
	var = grp->vars.value[i];
	if (!var) continue;
	if (varids)
	  varids[num_vars] = var->varid;
	num_vars++;
      }
   }

   /* If the user wants to know how many vars in the group, tell
    * him. */
   if (nvars)
      *nvars = num_vars;

   return NC_NOERR;
}

/* This is the comparison function used for sorting dim ids. Integer
   comparison: returns negative if b > a and positive if a > b. */
int int_cmp(const void *a, const void *b)
{
   const int *ia = (const int *)a; 
   const int *ib = (const int *)b;
   return *ia  - *ib; 
}

/* Find all dimids for a location. This finds all dimensions in a
 * group, with or without any of its parents, depending on last
 * parameter. */
int 
NC4_inq_dimids(int ncid, int *ndims, int *dimids, int include_parents)
{
   NC_GRP_INFO_T *grp, *g;
   NC_HDF5_FILE_INFO_T *h5;
   NC_DIM_INFO_T *dim;
   int d, num = 0;
   int retval;

   LOG((2, "nc_inq_dimids: ncid 0x%x include_parents: %d", ncid, 
	include_parents));

   /* Find info for this file and group, and set pointer to each. */
   if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
      return retval;

   if (!h5)
   {
      /* If this is a netcdf-3 file, then the dimids are going to be 0
       * thru ndims-1, so just provide them. */
      if ((retval = NC4_inq(ncid, &num, NULL, NULL, NULL)))
	 return retval;
      if (dimids)
	 for (d = 0; d < num; d++)
	    dimids[d] = d;
   }
   else
   {
      /* First count them. */
      for (dim = grp->dim; dim; dim = dim->l.next)
	 num++;
      if (include_parents)
	 for (g = grp->parent; g; g = g->parent)
	    for (dim = g->dim; dim; dim = dim->l.next)
	       num++;
      
      /* If the user wants the dimension ids, get them. */
      if (dimids)
      {
	 int n = 0;

	 /* Get dimension ids from this group. */
	 for (dim = grp->dim; dim; dim = dim->l.next)
	    dimids[n++] = dim->dimid;

	 /* Get dimension ids from parent groups. */
	 if (include_parents)
	    for (g = grp->parent; g; g = g->parent)
	       for (dim = g->dim; dim; dim = dim->l.next)
		  dimids[n++] = dim->dimid;
	 
	 qsort(dimids, num, sizeof(int), int_cmp);
      }
   }

   /* If the user wants the number of dims, give it. */
   if (ndims)
      *ndims = num;

   return NC_NOERR;
}

