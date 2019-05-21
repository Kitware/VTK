/*
This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc4 attribute functions.

Remember that with atts, type conversion can take place when writing
them, and when reading them.

Copyright 2003-2011, University Corporation for Atmospheric
Research. See COPYRIGHT file for copying and redistribution
conditions.
*/

#include "nc4internal.h"
#include "nc.h"
#include "nc4dispatch.h"
#include "ncdispatch.h"

static int nc4_get_att_special(NC_HDF5_FILE_INFO_T*, const char*,
                               nc_type*, nc_type, size_t*, int*, int, void*);

int nc4typelen(nc_type type);

/* Get or put attribute metadata from our linked list of file
   info. Always locate the attribute by name, never by attnum.
   The mem_type is ignored if data=NULL. */
int
nc4_get_att(int ncid, NC *nc, int varid, const char *name,
	    nc_type *xtype, nc_type mem_type, size_t *lenp,
	    int *attnum, int is_long, void *data)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_ATT_INFO_T *att = NULL;
   int my_attnum = -1;

   int need_to_convert = 0;
   int range_error = NC_NOERR;
   void *bufr = NULL;
   size_t type_size;
   char norm_name[NC_MAX_NAME + 1];
   int i;
   int retval = NC_NOERR;

   if (attnum) {
      my_attnum = *attnum;
   }

   LOG((3, "%s: ncid 0x%x varid %d name %s attnum %d mem_type %d",
	__func__, ncid, varid, name, my_attnum, mem_type));

   /* Find info for this file and group, and set pointer to each. */
   h5 = NC4_DATA(nc);
   if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
      BAIL(NC_EBADGRPID);

   /* Check varid */
   if (varid != NC_GLOBAL) {
       if (varid < 0 || varid >= grp->vars.nelems)
	   return NC_ENOTVAR;
       if (grp->vars.value[varid] == NULL)
           return NC_ENOTVAR;
       assert(grp->vars.value[varid]->varid == varid);
   }

   if (name == NULL)
       BAIL(NC_EBADNAME);

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      BAIL(retval);

   if(nc->ext_ncid == ncid && varid == NC_GLOBAL) {
	const char** sp;
	for(sp = NC_RESERVED_SPECIAL_LIST;*sp;sp++) {
	    if(strcmp(name,*sp)==0) {
		return nc4_get_att_special(h5, norm_name, xtype, mem_type, lenp, attnum, is_long, data);
	    }
	}
    }

   /* Find the attribute, if it exists.
      <strike>If we don't find it, we are major failures.</strike>
   */
   if ((retval = nc4_find_grp_att(grp, varid, norm_name, my_attnum, &att))) {
     if(retval == NC_ENOTATT)
	return retval;
     else
      BAIL(retval);
   }

   /* If mem_type is NC_NAT, it means we want to use the attribute's
    * file type as the mem type as well. */
   if (mem_type == NC_NAT)
      mem_type = att->nc_typeid;

   /* If the attribute is NC_CHAR, and the mem_type isn't, or vice
    * versa, that's a freakish attempt to convert text to
    * numbers. Some pervert out there is trying to pull a fast one!
    * Send him an NC_ECHAR error...*/
   if (data && att->len &&
       ((att->nc_typeid == NC_CHAR && mem_type != NC_CHAR) ||
	(att->nc_typeid != NC_CHAR && mem_type == NC_CHAR)))
      BAIL(NC_ECHAR); /* take that, you freak! */

   /* Copy the info. */
   if (lenp)
      *lenp = att->len;
   if (xtype)
      *xtype = att->nc_typeid;
   if (attnum) {
      *attnum = att->attnum;
   }

   /* Zero len attributes are easy to read! */
   if (!att->len)
      BAIL(NC_NOERR);

   /* Later on, we will need to know the size of this type. */
   if ((retval = nc4_get_typelen_mem(h5, mem_type, is_long, &type_size)))
      BAIL(retval);

   /* We may have to convert data. Treat NC_CHAR the same as
    * NC_UBYTE. If the mem_type is NAT, don't try any conversion - use
    * the attribute's type. */
   if (data && att->len && mem_type != att->nc_typeid &&
       mem_type != NC_NAT &&
       !(mem_type == NC_CHAR &&
	 (att->nc_typeid == NC_UBYTE || att->nc_typeid == NC_BYTE)))
   {
      if (!(bufr = malloc((size_t)(att->len * type_size))))
	 BAIL(NC_ENOMEM);
      need_to_convert++;
      if ((retval = nc4_convert_type(att->data, bufr, att->nc_typeid,
				     mem_type, (size_t)att->len, &range_error,
				     NULL, (h5->cmode & NC_CLASSIC_MODEL), 0, is_long)))
	 BAIL(retval);

      /* For strict netcdf-3 rules, ignore erange errors between UBYTE
       * and BYTE types. */
      if ((h5->cmode & NC_CLASSIC_MODEL) &&
	  (att->nc_typeid == NC_UBYTE || att->nc_typeid == NC_BYTE) &&
	  (mem_type == NC_UBYTE || mem_type == NC_BYTE) &&
	  range_error)
	 range_error = 0;
   }
   else
   {
      bufr = att->data;
   }

   /* If the caller wants data, copy it for him. If he hasn't
      allocated enough memory for it, he will burn in segmentation
      fault hell, writhing with the agony of undiscovered memory
      bugs! */
   if (data)
   {
      if (att->vldata)
      {
	 size_t base_typelen;
	 hvl_t *vldest = data;
	 NC_TYPE_INFO_T *type;

         /* Get the type object for the attribute's type */
	 if ((retval = nc4_find_type(h5, att->nc_typeid, &type)))
	   BAIL(retval);

         /* Retrieve the size of the base type */
         if ((retval = nc4_get_typelen_mem(h5, type->u.v.base_nc_typeid, 0, &base_typelen)))
            BAIL(retval);

	 for (i = 0; i < att->len; i++)
	 {
	    vldest[i].len = att->vldata[i].len;
	    if (!(vldest[i].p = malloc(vldest[i].len * base_typelen)))
	       BAIL(NC_ENOMEM);
	    memcpy(vldest[i].p, att->vldata[i].p, vldest[i].len * base_typelen);
	 }
      }
      else if (att->stdata)
      {
	 for (i = 0; i < att->len; i++)
	 {
            /* Check for NULL pointer for string (valid in HDF5) */
            if(att->stdata[i])
            {
                if (!(((char **)data)[i] = strdup(att->stdata[i])))
                   BAIL(NC_ENOMEM);
            }
            else
                ((char **)data)[i] = att->stdata[i];
	 }
      }
      else
      {
	 /* For long types, we need to handle this special... */
	 if (is_long && att->nc_typeid == NC_INT)
	 {
	    long *lp = data;
	    int *ip = bufr;

	    for (i = 0; i < att->len; i++)
	       *lp++ = *ip++;
	 }
	 else
	    memcpy(data, bufr, (size_t)(att->len * type_size));
      }
   }

 exit:
   if (need_to_convert)
      free(bufr);
   if (range_error)
      retval = NC_ERANGE;
   return retval;
}

/* Put attribute metadata into our global metadata. */
static int
nc4_put_att(int ncid, NC *nc, int varid, const char *name,
	    nc_type file_type, nc_type mem_type, size_t len, int is_long,
	    const void *data)
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var = NULL;
   NC_ATT_INFO_T *att, **attlist = NULL;
   char norm_name[NC_MAX_NAME + 1];
   nc_bool_t new_att = NC_FALSE;
   int retval = NC_NOERR, range_error = 0;
   size_t type_size;
   int i;
   int res;

   if (!name)
      return NC_EBADNAME;
   assert(nc && NC4_DATA(nc));

   LOG((1, "nc4_put_att: ncid 0x%x varid %d name %s "
	"file_type %d mem_type %d len %d", ncid, varid,
	name, file_type, mem_type, len));

   /* If len is not zero, then there must be some data. */
   if (len && !data)
      return NC_EINVAL;

   /* Find info for this file and group, and set pointer to each. */
   h5 = NC4_DATA(nc);
   if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
      return NC_EBADGRPID;

   /* If the file is read-only, return an error. */
   if (h5->no_write)
     return NC_EPERM;

   /* Find att, if it exists. */
   if (varid == NC_GLOBAL)
      attlist = &grp->att;
   else
   {
      if (varid < 0 || varid >= grp->vars.nelems)
	return NC_ENOTVAR;
      var = grp->vars.value[varid];
      if (!var) return NC_ENOTVAR;
      attlist = &var->att;
      assert(var->varid == varid);
   }

   if (!name)
      return NC_EBADNAME;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   if(nc->ext_ncid == ncid && varid == NC_GLOBAL) {
	const char** sp;
	for(sp = NC_RESERVED_SPECIAL_LIST;*sp;sp++) {
	    if(strcmp(name,*sp)==0) {
		return NC_ENOTATT; /* Not settable */
	    }
	}
    }

   for (att = *attlist; att; att = att->l.next)
     if (!strcmp(att->name, norm_name))
       break;

   /* If len is not zero, then there must be some data. */
   if (len && !data)
      return NC_EINVAL;

   LOG((1, "nc4_put_att: ncid 0x%x varid %d name %s "
	"file_type %d mem_type %d len %d", ncid, varid,
	name, file_type, mem_type, len));

   if (!att)
   {
      /* If this is a new att, require define mode. */
      if (!(h5->flags & NC_INDEF))
      {
	 if (h5->cmode & NC_CLASSIC_MODEL)
	    return NC_EINDEFINE;
	 if ((retval = NC4_redef(ncid)))
	    BAIL(retval);
      }
      new_att = NC_TRUE;
   }
   else
   {
      /* For an existing att, if we're not in define mode, the len
	 must not be greater than the existing len for classic model. */
     if (!(h5->flags & NC_INDEF) &&
         len * nc4typelen(file_type) > (size_t)att->len * nc4typelen(att->nc_typeid))
       {
         if (h5->cmode & NC_CLASSIC_MODEL)
           return NC_EINDEFINE;
         if ((retval = NC4_redef(ncid)))
           BAIL(retval);
       }
   }

   /* We must have two valid types to continue. */
   if (file_type == NC_NAT || mem_type == NC_NAT)
      return NC_EBADTYPE;

   /* Get information about this type. */
   if ((retval = nc4_get_typelen_mem(h5, file_type, is_long, &type_size)))
      return retval;

   /* No character conversions are allowed. */
   if (file_type != mem_type &&
       (file_type == NC_CHAR || mem_type == NC_CHAR ||
	file_type == NC_STRING || mem_type == NC_STRING))
      return NC_ECHAR;

   /* For classic mode file, only allow atts with classic types to be
    * created. */
   if (h5->cmode & NC_CLASSIC_MODEL && file_type > NC_DOUBLE)
      return NC_ESTRICTNC3;

   /* Add to the end of the attribute list, if this att doesn't
      already exist. */
   if (new_att)
   {
      LOG((3, "adding attribute %s to the list...", norm_name));
      if ((res = nc4_att_list_add(attlist, &att)))
        BAIL (res);
      if (!(att->name = strdup(norm_name)))
        return NC_ENOMEM;
   }

   /* Now fill in the metadata. */
   att->dirty = NC_TRUE;
   att->nc_typeid = file_type;

   /* If this att has vlen or string data, release it before we lose the length value. */
   if (att->stdata)
   {
      for (i = 0; i < att->len; i++)
         if(att->stdata[i])
	    free(att->stdata[i]);
      free(att->stdata);
      att->stdata = NULL;
   }
   if (att->vldata)
   {
      for (i = 0; i < att->len; i++)
	 nc_free_vlen(&att->vldata[i]);
      free(att->vldata);
      att->vldata = NULL;
   }

   att->len = len;
   if (att->l.prev)
      att->attnum = ((NC_ATT_INFO_T *)att->l.prev)->attnum + 1;
   else
      att->attnum = 0;

   /* If this is the _FillValue attribute, then we will also have to
    * copy the value to the fill_vlue pointer of the NC_VAR_INFO_T
    * struct for this var. (But ignore a global _FillValue
    * attribute). */
   if (!strcmp(att->name, _FillValue) && varid != NC_GLOBAL)
   {
      int size;

      /* Fill value must be same type and have exactly one value */
      if (att->nc_typeid != var->type_info->nc_typeid)
        return NC_EBADTYPE;
      if (att->len != 1)
        return NC_EINVAL;

      /* If we already wrote to the dataset, then return an error. */
      if (var->written_to)
        return NC_ELATEFILL;

      /* If fill value hasn't been set, allocate space. Of course,
       * vlens have to be different... */
      if ((retval = nc4_get_typelen_mem(grp->nc4_info, var->type_info->nc_typeid, 0,
                                        &type_size)))
        return retval;

      /* Already set a fill value? Now I'll have to free the old
       * one. Make up your damn mind, would you? */
      if (var->fill_value)
        {
          if (var->type_info->nc_type_class == NC_VLEN)
            {
              if ((retval = nc_free_vlen(var->fill_value)))
                return retval;
            }
          else if (var->type_info->nc_type_class == NC_STRING)
            {
              if (*(char **)var->fill_value)
                free(*(char **)var->fill_value);
            }
          free(var->fill_value);
        }

      /* Allocate space for the fill value. */
      if (var->type_info->nc_type_class == NC_VLEN)
        size = sizeof(hvl_t);
      else if (var->type_info->nc_type_class == NC_STRING)
        size = sizeof(char *);
      else
        size = type_size;

      if (!(var->fill_value = calloc(1, size)))
        return NC_ENOMEM;

      /* Copy the fill_value. */
      LOG((4, "Copying fill value into metadata for variable %s", var->name));
      if (var->type_info->nc_type_class == NC_VLEN)
        {
          nc_vlen_t *in_vlen = (nc_vlen_t *)data, *fv_vlen = (nc_vlen_t *)(var->fill_value);

          fv_vlen->len = in_vlen->len;
          if (!(fv_vlen->p = malloc(size * in_vlen->len)))
            return NC_ENOMEM;
          memcpy(fv_vlen->p, in_vlen->p, in_vlen->len * size);
        }
      else if (var->type_info->nc_type_class == NC_STRING)
        {
          if(NULL != (*(char **)data))
            {
              if (!(*(char **)(var->fill_value) = malloc(strlen(*(char **)data) + 1)))
                return NC_ENOMEM;
              strcpy(*(char **)var->fill_value, *(char **)data);
            }
          else
            *(char **)var->fill_value = NULL;
        }
      else
        memcpy(var->fill_value, data, type_size);

      /* Indicate that the fill value was changed, if the variable has already
       * been created in the file, so the dataset gets deleted and re-created. */
      if (var->created)
         var->fill_val_changed = NC_TRUE;
   }

   /* Copy the attribute data, if there is any. VLENs and string
    * arrays have to be handled specially. */
   if(att->len)
   {
      nc_type type_class;    /* Class of attribute's type */

      /* Get class for this type. */
      if ((retval = nc4_get_typeclass(h5, file_type, &type_class)))
         return retval;

      assert(data);
      if (type_class == NC_VLEN)
      {
         const hvl_t *vldata1;
	 NC_TYPE_INFO_T *type;
	 size_t base_typelen;

         /* Get the type object for the attribute's type */
	 if ((retval = nc4_find_type(h5, file_type, &type)))
	   BAIL(retval);

         /* Retrieve the size of the base type */
         if ((retval = nc4_get_typelen_mem(h5, type->u.v.base_nc_typeid, 0, &base_typelen)))
            BAIL(retval);

         vldata1 = data;
         if (!(att->vldata = (nc_vlen_t*)malloc(att->len * sizeof(hvl_t))))
            BAIL(NC_ENOMEM);
         for (i = 0; i < att->len; i++)
         {
            att->vldata[i].len = vldata1[i].len;
            if (!(att->vldata[i].p = malloc(base_typelen * att->vldata[i].len)))
               BAIL(NC_ENOMEM);
            memcpy(att->vldata[i].p, vldata1[i].p, base_typelen * att->vldata[i].len);
         }
      }
      else if (type_class == NC_STRING)
      {
        LOG((4, "copying array of NC_STRING"));
        if (!(att->stdata = malloc(sizeof(char *) * att->len))) {
          BAIL(NC_ENOMEM);
        }

        /* If we are overwriting an existing attribute,
           specifically an NC_CHAR, we need to clean up
           the pre-existing att->data. */
        if (!new_att && att->data) {
          free(att->data);
          att->data = NULL;
        }

        for (i = 0; i < att->len; i++)
          {
            if(NULL != ((char **)data)[i]) {
              LOG((5, "copying string %d of size %d", i, strlen(((char **)data)[i]) + 1));
              if (!(att->stdata[i] = strdup(((char **)data)[i])))
                BAIL(NC_ENOMEM);
            }
            else
              att->stdata[i] = ((char **)data)[i];
          }
      }
      else
      {
         /* [Re]allocate memory for the attribute data */
         if (!new_att)
            free (att->data);
         if (!(att->data = malloc(att->len * type_size)))
            BAIL(NC_ENOMEM);

         /* Just copy the data, for non-atomic types */
         if (type_class == NC_OPAQUE || type_class == NC_COMPOUND || type_class == NC_ENUM)
            memcpy(att->data, data, len * type_size);
         else
         {
            /* Data types are like religions, in that one can convert.  */
            if ((retval = nc4_convert_type(data, att->data, mem_type, file_type,
                                           len, &range_error, NULL,
                                           (h5->cmode & NC_CLASSIC_MODEL), is_long, 0)))
               BAIL(retval);
         }
      }
   }
   att->dirty = NC_TRUE;
   att->created = NC_FALSE;

   /* Mark attributes on variable dirty, so they get written */
   if(var)
       var->attr_dirty = NC_TRUE;

 exit:
   /* If there was an error return it, otherwise return any potential
      range error value. If none, return NC_NOERR as usual.*/
   if (retval)
      return retval;
   if (range_error)
      return NC_ERANGE;
   return NC_NOERR;
}

/* Learn about an att. All the nc4 nc_inq_ functions just call
 * nc4_get_att to get the metadata on an attribute. */
int
NC4_inq_att(int ncid, int varid, const char *name, nc_type *xtypep, size_t *lenp)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;

   LOG((2, "nc_inq_att: ncid 0x%x varid %d name %s", ncid, varid, name));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   /* get netcdf-4 metadata */
   h5 = NC4_DATA(nc);
   assert(h5);

   /* Handle netcdf-4 files. */
   return nc4_get_att(ncid, nc, varid, name, xtypep, NC_NAT, lenp, NULL, 0, NULL);
}

/* Learn an attnum, given a name. */
int
NC4_inq_attid(int ncid, int varid, const char *name, int *attnump)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;
   int stat;

   LOG((2, "nc_inq_attid: ncid 0x%x varid %d name %s", ncid, varid, name));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   /* get netcdf-4 metadata */
   h5 = NC4_DATA(nc);
   assert(h5);

   /* Handle netcdf-4 files. */
   stat = nc4_get_att(ncid, nc, varid, name, NULL, NC_NAT,
		      NULL, attnump, 0, NULL);
   return stat;
}


/* Given an attnum, find the att's name. */
int
NC4_inq_attname(int ncid, int varid, int attnum, char *name)
{
   NC *nc;
   NC_ATT_INFO_T *att;
   NC_HDF5_FILE_INFO_T *h5;
   int retval = NC_NOERR;

   LOG((2, "nc_inq_attname: ncid 0x%x varid %d attnum %d",
	ncid, varid, attnum));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   /* get netcdf-4 metadata */
   h5 = NC4_DATA(nc);
   assert(h5);

   /* Handle netcdf-4 files. */
   if ((retval = nc4_find_nc_att(ncid, varid, NULL, attnum, &att)))
      return retval;

   /* Get the name. */
   if (name)
      strcpy(name, att->name);

   return NC_NOERR;
}

/* I think all atts should be named the exact same thing, to avoid
   confusion! */
int
NC4_rename_att(int ncid, int varid, const char *name,
	      const char *newname)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var = NULL;
   NC_ATT_INFO_T *att, *list;
   char norm_newname[NC_MAX_NAME + 1], norm_name[NC_MAX_NAME + 1];
   hid_t datasetid = 0;
   int retval = NC_NOERR;

   if (!name || !newname)
      return NC_EINVAL;

   LOG((2, "nc_rename_att: ncid 0x%x varid %d name %s newname %s",
	ncid, varid, name, newname));

   /* If the new name is too long, that's an error. */
   if (strlen(newname) > NC_MAX_NAME)
      return NC_EMAXNAME;

   /* Find metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(h5 && grp);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
     return NC_EPERM;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(newname, norm_newname)))
      return retval;

   /* Is norm_newname in use? */
   if (varid == NC_GLOBAL)
   {
      list = grp->att;
   }
   else
   {
      if (varid < 0 || varid >= grp->vars.nelems)
	return NC_ENOTVAR;
      var = grp->vars.value[varid];
      if (!var) return NC_ENOTVAR;
      assert(var->varid == varid);
      list = var->att;
   }
   for (att = list; att; att = att->l.next)
      if (!strncmp(att->name, norm_newname, NC_MAX_NAME))
	 return NC_ENAMEINUSE;

   /* Normalize name and find the attribute. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;
   for (att = list; att; att = att->l.next)
      if (!strncmp(att->name, norm_name, NC_MAX_NAME))
	 break;
   if (!att)
      return NC_ENOTATT;

   /* If we're not in define mode, new name must be of equal or
      less size, if complying with strict NC3 rules. */
   if (!(h5->flags & NC_INDEF) && strlen(norm_newname) > strlen(att->name) &&
       (h5->cmode & NC_CLASSIC_MODEL))
      return NC_ENOTINDEFINE;

   /* Delete the original attribute, if it exists in the HDF5 file. */
   if (att->created)
   {
      if (varid == NC_GLOBAL)
      {
         if (H5Adelete(grp->hdf_grpid, att->name) < 0)
	    return NC_EHDFERR;
      }
      else
      {
	 if ((retval = nc4_open_var_grp2(grp, varid, &datasetid)))
	    return retval;
         if (H5Adelete(datasetid, att->name) < 0)
	    return NC_EHDFERR;
      }
      att->created = NC_FALSE;
   }

   /* Copy the new name into our metadata. */
   free(att->name);
   if (!(att->name = malloc((strlen(norm_newname) + 1) * sizeof(char))))
      return NC_ENOMEM;
   strcpy(att->name, norm_newname);
   att->dirty = NC_TRUE;

   /* Mark attributes on variable dirty, so they get written */
   if(var)
       var->attr_dirty = NC_TRUE;

   return retval;
}

/* Delete an att. Rub it out. Push the button on it. Liquidate
   it. Bump it off. Take it for a one-way ride. Terminate it. Drop the
   bomb on it. You get the idea.
   Ed Hartnett, 10/1/3
*/
int
NC4_del_att(int ncid, int varid, const char *name)
{
   NC *nc;
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_ATT_INFO_T *att, *natt;
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T **attlist = NULL;
   hid_t locid = 0, datasetid = 0;
   int retval = NC_NOERR;

   if (!name)
      return NC_EINVAL;

   LOG((2, "nc_del_att: ncid 0x%x varid %d name %s",
	ncid, varid, name));

   /* Find metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   assert(h5 && grp);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
      return NC_EPERM;

   /* If it's not in define mode, forget it. */
   if (!(h5->flags & NC_INDEF))
   {
      if (h5->cmode & NC_CLASSIC_MODEL)
	 return NC_ENOTINDEFINE;
      if ((retval = NC4_redef(ncid)))
	 BAIL(retval);
   }

   /* Get either the global or a variable attribute list. Also figure
      out the HDF5 location it's attached to. */
   if (varid == NC_GLOBAL)
   {
      attlist = &grp->att;
      locid = grp->hdf_grpid;
   }
   else
   {
      if (varid < 0 || varid >= grp->vars.nelems)
	return NC_ENOTVAR;
      var = grp->vars.value[varid];
      if (!var) return NC_ENOTVAR;
      attlist = &var->att;
      assert(var->varid == varid);
      if (var->created)
	 locid = var->hdf_datasetid;
   }

   /* Now find the attribute by name or number. */
   for (att = *attlist; att; att = att->l.next)
      if (!strcmp(att->name, name))
	 break;

   /* If att is NULL, we couldn't find the attribute. */
   if (!att)
      BAIL_QUIET(NC_ENOTATT);

   /* Delete it from the HDF5 file, if it's been created. */
   if (att->created)
   {
      assert(locid);

      if(H5Adelete(locid, att->name) < 0)
	 BAIL(NC_EATTMETA);
   }

   /* Renumber all following attributes. */
   for (natt = att->l.next; natt; natt = natt->l.next)
      natt->attnum--;

   /* Delete this attribute from this list. */
   if ((retval = nc4_att_list_del(attlist, att)))
      BAIL(retval);

 exit:
   if (datasetid > 0) H5Dclose(datasetid);
   return retval;
}

/* Write an attribute with type conversion. */
static int
nc4_put_att_tc(int ncid, int varid, const char *name, nc_type file_type,
	       nc_type mem_type, int mem_type_is_long, size_t len,
	       const void *op)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;

   /* The length needs to be positive (cast needed for braindead
      systems with signed size_t). */
   if((unsigned long) len > X_INT_MAX)
      return NC_EINVAL;

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   /* get netcdf-4 metadata */
   h5 = NC4_DATA(nc);
   assert(h5);

   /* Check varid */
   if (varid != NC_GLOBAL) {
       /* Find info for this file and group, and set pointer to each. */
       NC_GRP_INFO_T *grp;
       if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
          return NC_EBADGRPID;

       if (varid < 0 || varid >= grp->vars.nelems)
	   return NC_ENOTVAR;
       if (grp->vars.value[varid] == NULL)
           return NC_ENOTVAR;
       assert(grp->vars.value[varid]->varid == varid);
   }

   if (!name || strlen(name) > NC_MAX_NAME)
      return NC_EBADNAME;

   LOG((3, "nc4_put_att_tc: ncid 0x%x varid %d name %s file_type %d "
	"mem_type %d len %d", ncid, varid, name, file_type, mem_type, len));

   if(nc->ext_ncid == ncid && varid == NC_GLOBAL) {
      const char** reserved = NC_RESERVED_ATT_LIST;
      for(;*reserved;reserved++) {
	if(strcmp(name,*reserved)==0)
	    return NC_ENAMEINUSE;
      }
   }

   if(varid != NC_GLOBAL) {
      const char** reserved = NC_RESERVED_VARATT_LIST;
      for(;*reserved;reserved++) {
	if(strcmp(name,*reserved)==0)
	    return NC_ENAMEINUSE;
      }
   }

   /* Otherwise, handle things the netcdf-4 way. */
   return nc4_put_att(ncid, nc, varid, name, file_type, mem_type, len,
		      mem_type_is_long, op);
}

static int
nc4_get_att_special(NC_HDF5_FILE_INFO_T* h5, const char* name,
                    nc_type* filetypep, nc_type mem_type, size_t* lenp,
                    int* attnump, int is_long, void* data)
{
    /* Fail if asking for att id */
    if(attnump)
	return NC_EATTMETA;

    if(strcmp(name,NCPROPS)==0) {
	char* propdata = NULL;
	int stat = NC_NOERR;
	int len;
	if(h5->fileinfo->propattr.version == 0)
	    return NC_ENOTATT;
	if(mem_type == NC_NAT) mem_type = NC_CHAR;
	if(mem_type != NC_CHAR)
	    return NC_ECHAR;
	if(filetypep) *filetypep = NC_CHAR;
	stat = NC4_buildpropinfo(&h5->fileinfo->propattr, &propdata);
	if(stat != NC_NOERR) return stat;
	len = strlen(propdata);
	if(lenp) *lenp = len;
	if(data) strncpy((char*)data,propdata,len+1);
	free(propdata);
    } else if(strcmp(name,ISNETCDF4ATT)==0
              || strcmp(name,SUPERBLOCKATT)==0) {
	unsigned long long iv = 0;
	if(filetypep) *filetypep = NC_INT;
	if(lenp) *lenp = 1;
	if(strcmp(name,SUPERBLOCKATT)==0)
	    iv = (unsigned long long)h5->fileinfo->superblockversion;
	else /* strcmp(name,ISNETCDF4ATT)==0 */
	    iv = NC4_isnetcdf4(h5);
	if(mem_type == NC_NAT) mem_type = NC_INT;
	if(data)
	switch (mem_type) {
	case NC_BYTE: *((char*)data) = (char)iv; break;
	case NC_SHORT: *((short*)data) = (short)iv; break;
	case NC_INT: *((int*)data) = (int)iv; break;
	case NC_UBYTE: *((unsigned char*)data) = (unsigned char)iv; break;
	case NC_USHORT: *((unsigned short*)data) = (unsigned short)iv; break;
	case NC_UINT: *((unsigned int*)data) = (unsigned int)iv; break;
	case NC_INT64: *((long long*)data) = (long long)iv; break;
	case NC_UINT64: *((unsigned long long*)data) = (unsigned long long)iv; break;
	default:
	    return NC_ERANGE;
	}
    }
    return NC_NOERR;
}

/* Read an attribute of any type, with type conversion. This may be
 * called by any of the nc_get_att_* functions. */
int
nc4_get_att_tc(int ncid, int varid, const char *name, nc_type mem_type,
	       int mem_type_is_long, void *ip)
{
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;

   LOG((3, "nc4_get_att_tc: ncid 0x%x varid %d name %s mem_type %d",
	ncid, varid, name, mem_type));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid,NULL)))
      return NC_EBADID;

   /* get netcdf-4 metadata */
   h5 = NC4_DATA(nc);
   assert(h5);

   return nc4_get_att(ncid, nc, varid, name, NULL, mem_type,
		      NULL, NULL, mem_type_is_long, ip);
}

int
NC4_put_att(int ncid, int varid, const char *name, nc_type xtype,
	           size_t nelems, const void *value, nc_type memtype)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, memtype, 0, nelems, value);
}

int
NC4_get_att(int ncid, int varid, const char *name, void *value, nc_type memtype)
{
   return nc4_get_att_tc(ncid, varid, name, memtype, 0, value);
}
